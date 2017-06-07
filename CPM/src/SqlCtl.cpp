#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"
#include "SqlCtl.h"
#include "Define.h"
#include "Init.h"


#ifdef  DEBUG
#define DEBUG_DB
#endif

#ifdef DEBUG_DB
#define DBG_DB(a)		printf a;
#else
#define DBG_DB(a)	
#endif

#define DB_DIR "/home/CPM"
#define SQLCTL_TEMP_DB_PATH "/home/CPM/tmp.db"		//临时数据库
#define SQLCTL_BACKUP_DB_PATH "/home/CPM/cpm.db.bak"		//临时数据库
#define SQLCTL_HISTORY_DB_PATH "/home/CPM/cpm.db"		//历史数据库
#define SQLCTL_SAMPLE_DB_PATH "/home/CPM/sample.db"	//"标本数据库"

CSqlCtl::CSqlCtl(void)
{
	memset(m_dbPath, 0, sizeof(m_dbPath));
//	db = NULL;
}

CSqlCtl::~CSqlCtl(void)
{
	//close_database();
}

bool CSqlCtl::Initialize(const char* DBName)
{
	strcpy(m_dbPath, DBName);
	sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
	return true;
}

bool CSqlCtl::error(const char* error)
{	
	if (NULL != strstr(error, "database disk image is malformed")
		|| NULL != strstr(error, "no such table"))//数据库损坏，需修复
	{
		return true;
	}
	return false;
}
bool CSqlCtl::UpdateHistoryDB()
{
	bool ret = false;

	DBG_DB(("UpdateHistoryDB ... start\n"));
	DB_CONFIG dbConfig;
	getDBConfig(dbConfig);
	
	//生成统计数据lsd
	Ledger();

	CTblLockDb.lock();
	//备份数据到历史表lsd
	ret = UpdateDB(SQLCTL_TEMP_DB_PATH, SQLCTL_HISTORY_DB_PATH, dbConfig);	
	CTblLockDb.unlock();

	DBG_DB(("UpdateHistoryDB ... end\n"));
	return ret;
}

void CSqlCtl::getDBConfig(DB_CONFIG& dbConfig)
{
	memset((unsigned char*)&dbConfig, 0, sizeof(DB_CONFIG));
	char sql[512] = {0};

	char queryResult[128] = {0};
	if(0 == select_from_table(DB_TODAY_PATH, (char*)"config", (char*)" ColltMaxRecode, HoursMaxRecode, DaysMaxRecode, AlertMaxRecode,AlarmMaxRecode, AtsInfoMaxRecode, AtsLdgMaxRecode, ColltStoreMode", sql, queryResult))
	{
		char* split_result = NULL;
		char* savePtr = NULL;
		split_result = strtok_r(queryResult, "#", &savePtr);
		if (NULL != split_result)
		{
			dbConfig.iDataMaxCnt = atoi(split_result)*10000;
		}
		split_result = strtok_r(NULL, "#", &savePtr);
		if (NULL != split_result)
		{
			dbConfig.iHourLedgerMaxCnt = atoi(split_result)*10000;
		}

		split_result = strtok_r(NULL, "#", &savePtr);
		if (NULL != split_result)
		{
			dbConfig.iDayLedgerMaxCnt = atoi(split_result)*10000;
		}

		split_result = strtok_r(NULL, "#", &savePtr);
		if (NULL != split_result)
		{
			dbConfig.iAlarmMaxCnt = atoi(split_result)*10000;
		}

		split_result = strtok_r(NULL, "#", &savePtr);
		if (NULL != split_result)
		{
			dbConfig.iAlertMaxCnt = atoi(split_result)*10000;
		}
		split_result = strtok_r(NULL, "#", &savePtr);
		if (NULL != split_result)
		{
			dbConfig.iAtsInfoMaxRecode = atoi(split_result)*10000;
		}
		split_result = strtok_r(NULL, "#", &savePtr);
		if (NULL != split_result)
		{
			dbConfig.iAtsLdgMaxRecode = atoi(split_result)*10000;
		}
		split_result = strtok_r(NULL, "#", &savePtr);
		if (NULL != split_result)
		{
			dbConfig.iDataStoreMode = atoi(split_result);
		}
	}
	else
	{
		dbConfig.iDataMaxCnt = 30*10000;
		dbConfig.iHourLedgerMaxCnt = 50000;
		dbConfig.iDayLedgerMaxCnt = 50000;
		dbConfig.iAlarmMaxCnt = 10000;
		dbConfig.iAlertMaxCnt = 10000;
		dbConfig.iAtsInfoMaxRecode = 10000;
		dbConfig.iAtsLdgMaxRecode = 10000;
		dbConfig.iDataStoreMode = 0;
	}
	DBG_DB(("iDataMaxCnt[%d] iHourLedgerMaxCnt[%d] iDayLedgerMaxCnt[%d] iAlarmMaxCnt[%d] iAlertMaxCnt[%d] iAtsInfoMaxRecode[%d] iAtsLdgMaxRecode[%d]\n", 
		dbConfig.iDataMaxCnt, dbConfig.iHourLedgerMaxCnt ,dbConfig.iDayLedgerMaxCnt, dbConfig.iAlarmMaxCnt, dbConfig.iAlertMaxCnt, dbConfig.iAtsInfoMaxRecode, dbConfig.iAtsLdgMaxRecode));
}
//
bool CSqlCtl::Ledger()
{
	bool ret = false;	
	
	time_t tTime;
	time(&tTime); 
	tTime -= ONE_DAY_SECONDS;
	struct tm* pTmNow  = localtime(&tTime);
	struct tm tmNow;
	memcpy((unsigned char*)&tmNow, (unsigned char*)pTmNow, sizeof(struct tm));

	doAvgLedger(tmNow);

	doCountLedger(tmNow);
	
	doTimeCheckLedger(tmNow);

	return ret;
}

int CSqlCtl::GetLedgerDeviceCallBack(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{
	PLEDGER_DEVICE_INFO stParaInfo = (PLEDGER_DEVICE_INFO)data;
	if (512 > stParaInfo->iDevCnt)
	{
		strncpy(stParaInfo->stLedgerDevice[stParaInfo->iDevCnt].id, column_value[0], DEVICE_ID_LEN);
		strncpy(stParaInfo->stLedgerDevice[stParaInfo->iDevCnt].attrId, column_value[1], DEVICE_SN_LEN);
		stParaInfo->iDevCnt++;
	}
	
	return 0;
}
int CSqlCtl::GetLedgerUserCallBack(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{
	PLEDGER_USER_INFO stParaInfo = (PLEDGER_USER_INFO)data;
	if (512 > stParaInfo->iCnt)
	{
		strcpy(stParaInfo->szUserId[stParaInfo->iCnt], column_value[0]);
		stParaInfo->iCnt++;
	}	
	return 0;
}

bool CSqlCtl::doAvgLedger(struct tm tmNow)
{
	LEDGER_DEVICE_INFO stLedgerDevice;
	memset((unsigned char*)&stLedgerDevice, 0, sizeof(LEDGER_DEVICE_INFO));

	//生成统计类数据平均值表
	const char* format = "select data.id, data.ctype"
		" from data, device_attr "
		" where substr(data.id, 1, %d) = device_attr.id "
		" and data.ctype = device_attr.sn "
		" and device_attr.ledger = '1'"
		" group by data.id, data.ctype;";

	char szDayTime[20] = {0};
	sprintf(szDayTime, "%04d-%02d-%02d", tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday);

	char sql[1024] = {0};
	sprintf(sql, format, DEVICE_TYPE_LEN);

	//查询数据库， 获取现有数据表中的设备属性信息
	if(0 == select_from_tableEx(sql, CSqlCtl::GetLedgerDeviceCallBack, (void *)&stLedgerDevice))
	{
		char sql[512] = {0};
		memset(sql, 0, sizeof(sql));
		for (int i=0; i<stLedgerDevice.iDevCnt; i++)
		{
			//日均值
			memset(sql, 0, sizeof(sql));
			const char* sqlFormat = " where id = '%s' and ctype = '%s' and ctime > '%s 00:00:00' and ctime < '%s 23:59:59'";
			sprintf(sql, sqlFormat, stLedgerDevice.stLedgerDevice[i].id, stLedgerDevice.stLedgerDevice[i].attrId,
				szDayTime, szDayTime);

			char queryResult[128] = {0};
			if(0 == select_from_table(DB_TODAY_PATH, (char*)"data", (char*)" round(avg(value), 2)", sql, queryResult))
			{
				char* split_result = NULL;
				char* savePtr = NULL;
				split_result = strtok_r(queryResult, "#", &savePtr);
				if (NULL != split_result)
				{
					sqlFormat = "insert into DATA_DAYSAVG(sn, id, ctype, ctime, value) "
						"values(NULL, '%s', '%s', '%s', '%g');";
					memset(sql, 0, sizeof(sql));
					sprintf(sql, sqlFormat, stLedgerDevice.stLedgerDevice[i].id, stLedgerDevice.stLedgerDevice[i].attrId, szDayTime, atof(split_result));
					ExecSql(sql, NULL, NULL, true, 0);
				}
			}

			//小时均值
			for (int j=0; j<24; j++)
			{
				memset(sql, 0, sizeof(sql));
				sqlFormat = " where id = '%s' and ctype = '%s' and ctime > '%s %02d:00:00' and ctime < '%s %02d:59:59'";
				sprintf(sql, sqlFormat, stLedgerDevice.stLedgerDevice[i].id, stLedgerDevice.stLedgerDevice[i].attrId,
					szDayTime, j,
					szDayTime, j);
				char queryDay[128] = {0};
				if(0 == select_from_table(DB_TODAY_PATH, (char*)"data", (char*)" round(avg(value), 2)", sql, queryDay))
				{
					char szHourTime[20] = {0};
					sprintf(szHourTime, "%s %02d:00:00", szDayTime, j);
					char* split_result = NULL;
					char* savePtr = NULL;
					split_result = strtok_r(queryDay, "#", &savePtr);
					if (NULL != split_result)
					{
						sqlFormat = "insert into DATA_HourAVG(sn, id, ctype, ctime, value) "
							"values(NULL, '%s', '%s', '%s', '%g');";
						memset(sql, 0, sizeof(sql));
						sprintf(sql, sqlFormat, stLedgerDevice.stLedgerDevice[i].id, stLedgerDevice.stLedgerDevice[i].attrId, szHourTime, atof(split_result));
						ExecSql(sql, NULL, NULL, true, 0);
					}
				}
				else
				{
					DBG(("doAvgLedger Error\n"));
				}
			}
		}
	}	
	return true;
}

bool CSqlCtl::doCountLedger(struct tm tmNow)
{
	LEDGER_DEVICE_INFO stLedgerDevice;
	memset((unsigned char*)&stLedgerDevice, 0, sizeof(LEDGER_DEVICE_INFO));

	//生成统计类数据平均值表
	const char* sqlFormat = "select data.id, data.ctype"
		" from data, device_attr "
		" where substr(data.id, 1, %d) = device_attr.id "
		" and data.ctype = device_attr.sn "
		" and device_attr.ledger <> '1'"
		" group by data.id, data.ctype;";

	char szDayTime[20] = {0};
	sprintf(szDayTime, "%04d-%02d-%02d", tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday);


	char sql[1024] = {0};
	sprintf(sql, sqlFormat, DEVICE_TYPE_LEN);
	//查询数据库， 获取现有数据表中的设备属性信息
	if(0 == select_from_tableEx(sql, CSqlCtl::GetLedgerDeviceCallBack, (void *)&stLedgerDevice))
	{
		memset(sql, 0, sizeof(sql));
		for (int i=0; i<stLedgerDevice.iDevCnt; i++)
		{
			//日条数
			memset(sql, 0, sizeof(sql));
			sqlFormat = " where id = '%s' and ctype = '%s' and ctime > '%s 00:00:00' and ctime < '%s 23:59:59'";
			sprintf(sql, sqlFormat, stLedgerDevice.stLedgerDevice[i].id, stLedgerDevice.stLedgerDevice[i].attrId,
				szDayTime, 
				szDayTime);

			char queryResult[128] = {0};
			if(0 == select_from_table(DB_TODAY_PATH, (char*)"data", (char*)" count(*)", sql, queryResult))
			{
				char* split_result = NULL;
				char* savePtr = NULL;
				split_result = strtok_r(queryResult, "#", &savePtr);
				if (NULL != split_result)
				{
					sqlFormat = "insert into DATA_DAYSAVG(sn, id, ctype, ctime, value) "
						"values(NULL, '%s', '%s', '%s', '%d');";
					memset(sql, 0, sizeof(sql));
					sprintf(sql, sqlFormat, stLedgerDevice.stLedgerDevice[i].id, stLedgerDevice.stLedgerDevice[i].attrId, szDayTime, atoi(split_result));
					ExecSql(sql, NULL, NULL, true, 0);
				}
			}

			//小时条数
			for (int j=0; j<24; j++)
			{
				memset(sql, 0, sizeof(sql));
				sqlFormat = " where id = '%s' and ctype = '%s' and ctime > '%s %02d:00:00' and ctime < '%s %02d:59:59'";
				sprintf(sql, sqlFormat, stLedgerDevice.stLedgerDevice[i].id, stLedgerDevice.stLedgerDevice[i].attrId,
					szDayTime, j,
					szDayTime, j);
				char queryDay[128] = {0};
				if(0 == select_from_table(DB_TODAY_PATH, (char*)"data", (char*)" count(*)", sql, queryDay))
				{
					char szHourTime[20] = {0};
					sprintf(szHourTime, "%s %02d:00:00", szDayTime, j);
					char* split_result = NULL;
					char* savePtr = NULL;
					split_result = strtok_r(queryDay, "#", &savePtr);
					if (NULL != split_result)
					{
						sqlFormat = "insert into DATA_HourAVG(sn, id, ctype, ctime, value) "
							"values(NULL, '%s', '%s', '%s', '%d');";
						memset(sql, 0, sizeof(sql));
						sprintf(sql, sqlFormat, stLedgerDevice.stLedgerDevice[i].id, stLedgerDevice.stLedgerDevice[i].attrId, szHourTime, atoi(split_result));
						ExecSql(sql, NULL, NULL, true, 0);
					}
				}
			}
		}
	}

	return true;
}

//考勤统计
bool CSqlCtl::doTimeCheckLedger(struct tm tmNow)
{
	char szStartTime[10] = {0};
	char szEndTime[10] = {0};
	char cBeLate = '0';
	char cBeLeave = '0';
	char cBeAway = '1';
	char szR_StartTime[10] = {0}; 
	char szR_EndTime[10] = {0}; 

	DBG(("Start TimeCheckLedger ....\n"));
	char szDayTime[20] = {0};
	sprintf(szDayTime, "%04d-%02d-%02d", tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday);

	//获取系统标准考勤时间
	char queryResult[128] = {0};
	if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"corp_info", (char*)"ats", (char*)"", queryResult))
		return false;

	char* savePtr = NULL;
	char* split_result = strtok_r(queryResult, "#", &savePtr);
	if (NULL == split_result)
		return false;

	char* pTime = strtok_r(split_result, ",", &savePtr);
	if (NULL == pTime)
		return false;

	strcpy(szStartTime, pTime);
	pTime = strtok_r(NULL, ",", &savePtr);
	if (NULL == pTime)
		return false;
	strcpy(szEndTime, pTime);
	if(5 != strlen(trim(szStartTime, strlen(szStartTime))) || 5 != strlen(trim(szEndTime, strlen(szEndTime))))
		return false;

	DBG(("Start TimeCheckLedger ....szStartTime[%s] szEndTime[%s]\n", szStartTime, szEndTime));
	//获取用户表

	LEDGER_USER_INFO stUsrInfo;
	memset((unsigned char*)&stUsrInfo, 0, sizeof(LEDGER_USER_INFO));

	const char* sqlFormat = "select sys_id"
		" from user_info where status = '0'";

	char sql[1024] = {0};
	strcpy(sql, sqlFormat);
	//按用户表统计
	if(0 == select_from_tableEx(sql, CSqlCtl::GetLedgerUserCallBack, (void *)&stUsrInfo))
	{
		for (int i=0; i<stUsrInfo.iCnt; i++)
		{
			//获取最早和最晚打卡时间
			memset(sql, 0, sizeof(sql));
			memset(szR_StartTime, 0, sizeof(szR_StartTime));
			memset(szR_EndTime, 0, sizeof(szR_EndTime));
			if(strlen(stUsrInfo.szUserId[i]) <= 0)
				continue;
			sqlFormat = " where u_id = '%s' and ctime > '%s 00:00:00' and ctime < '%s 23:59:59'";
			sprintf(sql, sqlFormat, 
				stUsrInfo.szUserId[i],
				szDayTime, 
				szDayTime);
			memset(queryResult, 0, sizeof(queryResult));
			if(0 == select_from_table(DB_TODAY_PATH, (char*)"ats", (char*)" min(ctime), max(ctime)", sql, queryResult))
			{
				split_result = NULL;
				savePtr = NULL;
				split_result = strtok_r(queryResult, "#", &savePtr);
				if (NULL != split_result)
				{
					if(19 == strlen(split_result))
						strncpy(szR_StartTime, split_result + 11, 5);
				}
				split_result = strtok_r(NULL, "#", &savePtr);
				if (NULL != split_result)
				{
					if(19 == strlen(split_result))
						strncpy(szR_EndTime, split_result + 11, 5);
				}
				if(5 != strlen(szR_StartTime) && 5 != strlen(szR_EndTime))
				{
					cBeLate = '0';
					cBeLeave = '0';
					cBeAway = '1';
				}
				else 
				{
					cBeAway = '0';//已打卡
					if(strcmp(szR_StartTime, szStartTime) <= 0)
					{
						cBeLate = '1';
					}
					else
					{
						cBeLate = '0';
					}
					if(strcmp(szR_EndTime, szEndTime) >= 0)
					{
						cBeLeave = '1';
					}		
					else
						cBeLeave = '0';
				}
			}
			else
			{
				cBeLate = '0';
				cBeLeave = '0';
				cBeAway = '1';
			}
			//插入到ATS_RESULT表
			sqlFormat = "insert into ats_result(sn, id, ctime, r_on_time, on_time, r_off_time, off_time, belate, beleave, beaway) "
				"values(NULL, '%s', '%s', '%s', '%s', '%s', '%s', %c, %c, %c);";
			memset(sql, 0, sizeof(sql));
			sprintf(sql, sqlFormat, stUsrInfo.szUserId[i], szDayTime, szStartTime, szR_StartTime, szEndTime, szR_EndTime, cBeLate, cBeLeave, cBeAway);
			ExecSql(sql, NULL, NULL, true, 0);
		}
	}

	return true;
}
/******************************************************************************/
/* Function Name  : UpdateDB                                                  */
/* Description    : 更新表数据		                                          */
/* Input          : const char *src       源数据库                            */
/*                  const char *dst       目的数据库						  */
/* Output         : none                                                      */
/* Return         : true    操作成功                                          */
/*                  false   操作失败                                          */
/******************************************************************************/

bool CSqlCtl::UpdateDB(const char* src, const char* dst, DB_CONFIG dbConfig)
{
	int ret = false;
	char tempdb[64] = {0};
	char syscmd[128] = {0};
	int retyr_cnt = 3;
	printf("UpdateDB ....\n");
	do 
	{
		//历史数据库备份
		int cmdRet = 0;
		strcpy(tempdb, SQLCTL_BACKUP_DB_PATH);
		memset(syscmd, 0, sizeof(syscmd));
		sprintf(syscmd, "rm -r %s", tempdb);
		cmdRet = system(syscmd);
		if(0 > cmdRet)
		{
			printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
			//return false;
		}

		memset(syscmd, 0, sizeof(syscmd));
		sprintf(syscmd, "cp -r %s %s", dst, tempdb);
		cmdRet = system(syscmd);
		if(0 > cmdRet)
		{
			printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
			return false;
		}

		//配置表同步
		while(retyr_cnt-- > 0)
		{
			//拷贝基础数据		
			if(!UpdateTable(src, tempdb, "act_limit"))
				continue;
			if(!UpdateTable(src, tempdb, "call"))
				continue;
			if(!UpdateTable(src, tempdb, "card_manager"))
				continue;
			if(!UpdateTable(src, tempdb, "config"))
				continue;
			if(!UpdateTable(src, tempdb, "corp_info"))
				continue;
			if(!UpdateTable(src, tempdb, "data_type"))
				continue;
			if(!UpdateTable(src, tempdb, "device_agent"))
				continue;
			if(!UpdateTable(src, tempdb, "device_detail"))
				continue;
			if(!UpdateTable(src, tempdb, "device_info"))
				continue;
			if(!UpdateTable(src, tempdb, "device_roll"))
				continue;
			if(!UpdateTable(src, tempdb, "device_attr"))
				continue;
			if(!UpdateTable(src, tempdb, "fp_info"))
				continue;
			if(!UpdateTable(src, tempdb, "manufacturer"))
				continue;
			if(!UpdateTable(src, tempdb, "role"))
				continue;
			if(!UpdateTable(src, tempdb, "user_info"))
				continue;
			if(!UpdateTable(src, tempdb, "task_info"))
				continue;
			if(!UpdateTable(src, tempdb, "device_cmd"))
				continue;
			if(!UpdateTable(src, tempdb, "device_act"))
				continue;
			
			ret = true;
			break;
		}
		if (!ret)
		{
			break;
		}

		//数据表同步
		while(retyr_cnt-- > 0)
		{
			//拷贝基础数据	
			if(!IncrementTable(src, tempdb, "ALARM_INFO", "ALARM_INFO", dbConfig.iAlarmMaxCnt))
				continue;
			if(!IncrementTable(src, tempdb, "ALERT_INFO", "ALERT_INFO", dbConfig.iAlertMaxCnt))
				continue;
			if(!IncrementTable(src, tempdb, "DATA_DAYSAVG", "DATA_DAYSAVG", dbConfig.iDayLedgerMaxCnt))
				continue;
			if(!IncrementTable(src, tempdb, "DATA_HOURAVG", "DATA_HOURAVG", dbConfig.iHourLedgerMaxCnt))
				continue;
			if(!IncrementTable(src, tempdb, "ATS", "ATS", dbConfig.iAtsInfoMaxRecode))
				continue;
			if(!IncrementTable(src, tempdb, "ATS_RESULT", "ATS_RESULT", dbConfig.iAtsLdgMaxRecode))
				continue;


			//备份data表，单独写 lsd（分表）
			if(!IncrementData(src, tempdb, dbConfig))
				continue;
			ret = true;
			break;
		}
	} while (false);
	if (!ret)//不成功，则恢复
	{
		printf("增量备份不成功，恢复\n");
		memset(syscmd, 0, sizeof(syscmd));
		sprintf(syscmd, "rm -rf %s", tempdb);
		if(0 > system(syscmd))
		{
			printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
			return false;
		}
	}
	else
	{
		printf("备份成功\n");
		memset(syscmd, 0, sizeof(syscmd));
		sprintf(syscmd, "mv %s %s", tempdb, dst);
		if(0 > system(syscmd))
		{
			printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
			return false;
		}
	}
	return ret;
}

/******************************************************************************/
/* Function Name  : UpdateTable                                               */
/* Description    : 表更新													  */
/* Input          : const char *src       源数据库                            */
/*                  const char *dst       目的数据库						  */
/*                  const char *tablename 表名称						      */
/* Output         : none                                                      */
/* Return         : true    操作成功                                          */
/*                  false   操作失败                                          */
/******************************************************************************/
bool CSqlCtl::UpdateTable(const char* src, const char* dst, const char* tablename)
{
	char syscmd[128] = {0};

	//导出临时表数据
	const char* export_format = "sqlite3 %s \"select * from %s\" > %s/%s.dat";
	sprintf(syscmd, export_format, src, tablename, DB_DIR, tablename);
	printf("%s\n", syscmd);
	if(0 > system(syscmd))
	{
		printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
		return false;
	}

	//删除历史表中数据
	const char* delete_format = "sqlite3 %s \"delete from %s;\"";
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, delete_format, dst, tablename);
	printf("%s\n", syscmd);
	if(0 > system(syscmd))
	{
		printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
		return false;
	}

	//将数据导入到历史表
	const char* import_format = "sqlite3 %s \".import %s/%s.dat %s\"";
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, import_format, dst, DB_DIR, tablename, tablename);
	printf("%s\n", syscmd);
	if(0 > system(syscmd))
	{
		printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
		return false;
	}

	//删除临时导出文件
	const char* delete_f = "rm -rf %s/%s.dat";
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, delete_f, DB_DIR, tablename);
	printf("%s\n", syscmd);
	if(0 > system(syscmd))
	{
		printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
		return false;
	}

	return true;
}

/******************************************************************************/
/* Function Name  : IncrementTable                                            */
/* Description    : 表更新(添加数据）										  */
/* Input          : const char *src       源数据库                            */
/*                  const char *dst       目的数据库						  */
/*                  const char *tablename 表名称						      */
/* Output         : none                                                      */
/* Return         : true    操作成功                                          */
/*                  false   操作失败                                          */
/******************************************************************************/
bool CSqlCtl::IncrementTable(const char* src, const char* dst, const char* tablename, const char* tarTableName, int record_Max)
{
	char syscmd[128] = {0};
	time_t tTime;
	time(&tTime); 
	struct tm* tmNow  = localtime(&tTime);

	//导出临时表数据
	const char* export_format = "sqlite3 %s \"select * from %s where ctime < '%04d-%02d-%02d 00:00:00';\" > %s/%s.dat";
	sprintf(syscmd, export_format, src, tablename, 1900 + tmNow->tm_year, tmNow->tm_mon + 1, tmNow->tm_mday, DB_DIR, tablename);
	printf("%s\n", syscmd);
	if(0 > system(syscmd))
	{
		printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
		return false;
	}

	//导入到历史表
	const char* import_format = "sqlite3 %s \".import %s/%s.dat %s\"";
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, import_format, dst, DB_DIR, tablename, tarTableName);
	printf("%s\n", syscmd);
	if(0 > system(syscmd))
	{
		printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
		return false;
	}

	//删除临时导出文件
	const char* delete_f = "rm -rf %s/%s.dat";
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, delete_f, DB_DIR, tablename);
	printf("%s\n", syscmd);
	if(0 > system(syscmd))
	{
		printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
		return false;
	}

	//将临时表数据删除
	const char* delete_format = "sqlite3 %s \"delete from %s where ctime < '%04d-%02d-%02d 00:00:00';\"";
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, delete_format, src, tablename, 1900 + tmNow->tm_year, 1 + tmNow->tm_mon, tmNow->tm_mday);
	printf("%s\n", syscmd);
	if(0 > system(syscmd))
	{
		printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
		return false;
	}

	//保留最新record_Max条记录
	const char* reserv_format = "sqlite3 %s \"delete from %s where sn not in (select sn from %s order by ctime desc limit 0,%d);\"";
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, reserv_format, dst, tarTableName, tarTableName, record_Max);
	printf("%s\n", syscmd);
	if(0 > system(syscmd))
	{
		printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
		return false;
	}

	return true;
}
/******************************************************************************/
/* Function Name  : IncrementData	                                          */
/* Description    : 表更新(添加数据）lsd									  */
/* Input          : const char *src       源数据库                            */
/*                  const char *dst       目的数据库						  */
/*                  const char *tablename 表名称						      */
/* Output         : none                                                      */
/* Return         : true    操作成功                                          */
/*                  false   操作失败                                          */
/******************************************************************************/
bool CSqlCtl::IncrementData(const char* src, const char* dst, DB_CONFIG dbConfig)
{
	char DB_Name[15] = {0};
	char dbName[15] = {0};
	const char* formatDB = "CREATE TABLE %s\n"
		"(\n"
		"  SN     integer PRIMARY KEY autoincrement,\n"
		"  ID     VARCHAR2(10)  default '',\n"
		"  CTYPE  VARCHAR2(4)   default '',\n"
		"  CTIME  DATETIME,\n"
		"  VALUE  VARCHAR2(128) default '',\n"
		"  STATUS VARCHAR2(1)   default '0',\n"
		"  FLAG   VARCHAR2(1)   default '0',\n"
		"  LEVEL  VARCHAR2(4)   default '',\n"
		"  DESC   VARCHAR2(128) default ''\n"
		" );";
	const char* formatIndex = "CREATE INDEX %s_index on %s(id, ctype, ctime);";
	const char* formatInsert = " Insert into data_table "
		" (tname, count, status) values "
		" ('%s', '%s', '%s'); ";
	const char* formatUpdate = " Update data_table "
		" set count = '%d', status='%s' "
		" where tname = '%s'; ";
	const char* formatDel = " Delete from %s; ";

	time_t tTime;
	time(&tTime); 
	tTime -= ONE_DAY_SECONDS;
	struct tm* pTmNow  = localtime(&tTime);
	struct tm tmNow;
	memcpy((unsigned char*)&tmNow, (unsigned char*)pTmNow, sizeof(struct tm));

	switch (dbConfig.iDataStoreMode)
	{
	case 0://data一张表
		{

			sprintf(DB_Name, "DATA");
			sprintf(dbName, "data");
		}
		break;
	case 1://每月 上中下旬,统计删几张表
		{
			if (11 > tmNow.tm_mday)
			{
				sprintf(DB_Name, "DATA_%04d%02d01", tmNow.tm_year + 1900, tmNow.tm_mon+1);
				sprintf(dbName, "data_%04d%02d01", tmNow.tm_year + 1900, tmNow.tm_mon+1);
			}
			else if (21 > tmNow.tm_mday)
			{
				sprintf(DB_Name, "DATA_%04d%02d11", tmNow.tm_year + 1900, tmNow.tm_mon+1);
				sprintf(dbName, "data_%04d%02d11", tmNow.tm_year + 1900, tmNow.tm_mon+1);
			}
			else
			{
				sprintf(DB_Name, "DATA_%04d%02d21", tmNow.tm_year + 1900, tmNow.tm_mon+1);
				sprintf(dbName, "data_%04d%02d21", tmNow.tm_year + 1900, tmNow.tm_mon+1);
			}

			char sqlResult[56] = {0};
			char* split_result = NULL;
			char* savePtr = NULL;
			char condition[128] = {0};
			sprintf(condition, "where tname = '%s'", dbName);
			select_from_table_no_lock(SQLCTL_HISTORY_DB_PATH, (char*)"data_table", (char*)"COUNT(*)", condition, sqlResult);
			split_result = strtok_r(sqlResult, "#", &savePtr);

			if(atoi(split_result) <1)
			{

				//创建数据库
				char sql[1024] = {0};  
				sprintf(sql, formatDB, DB_Name);
				CPM_ExecSql(sql, NULL, NULL, true, 0);

				//创建索引
				memset(sql, 0, sizeof(sql));
				sprintf(sql, formatIndex, dbName, dbName);
				CPM_ExecSql(sql, NULL, NULL, true, 0);

				//插入data_table
				memset(sql, 0, sizeof(sql));
				sprintf(sql, formatInsert, dbName, "0", "0");
				CPM_ExecSql(sql, NULL, NULL, true, 0);
			}
		}
		break;
	case 2://每月 上半月、下半月
		{
			if (16 > tmNow.tm_mday)
			{
				sprintf(DB_Name, "DATA_%04d%02d01", tmNow.tm_year + 1900, tmNow.tm_mon+1);
				sprintf(dbName, "data_%04d%02d01", tmNow.tm_year + 1900, tmNow.tm_mon+1);
			}
			else
			{
				sprintf(DB_Name, "DATA_%04d%02d16", tmNow.tm_year + 1900, tmNow.tm_mon+1);
				sprintf(dbName, "data_%04d%02d16", tmNow.tm_year + 1900, tmNow.tm_mon+1);
			}

			char sqlResult[56] = {0};
			char* split_result = NULL;
			char* savePtr = NULL;
			char condition[128] = {0};
			sprintf(condition, "where tname = '%s'", dbName);
			select_from_table_no_lock(SQLCTL_HISTORY_DB_PATH, (char*)"data_table", (char*)"COUNT(*)", condition, sqlResult);
			split_result = strtok_r(sqlResult, "#", &savePtr);

			if(atoi(split_result) <1)
			{
				//创建数据库
				char sql[1024] = {0};  
				sprintf(sql, formatDB, DB_Name);
				CPM_ExecSql(sql, NULL, NULL, true, 0);

				//创建索引
				memset(sql, 0, sizeof(sql));
				sprintf(sql, formatIndex, dbName, dbName);
				CPM_ExecSql(sql, NULL, NULL, true, 0);

				//插入data_table
				memset(sql, 0, sizeof(sql));
				sprintf(sql, formatInsert, dbName, "0", "0");
				CPM_ExecSql(sql, NULL, NULL, true, 0);
			}
		}
		break;
	}
	//备份

	IncrementTable(src, dst, "DATA", dbName, dbConfig.iDataMaxCnt);
	//更新DATA_TABLE & 删除超出部分
	char sqlResult[56] = {0};
	char* split_result = NULL;
	char* savePtr = NULL;
	//当前操作表条数
	int currCnt = 0;
	select_from_table_no_lock(SQLCTL_BACKUP_DB_PATH, dbName, (char*)"COUNT(*)", (char*)"", sqlResult);
	split_result = strtok_r(sqlResult, "#", &savePtr);
	currCnt = atoi(split_result);
	//更新DATA_TABLE
	char sql[256] = {0};
	sprintf(sql, formatUpdate, currCnt, "0", dbName);
	CPM_ExecSql(sql, NULL, NULL, true, 0);
	//删除
	bool bCntMax = false;
	int  totalCnt = 0;
	do
	{
		memset(sqlResult, 0, 56);
		select_from_table_no_lock(SQLCTL_BACKUP_DB_PATH, (char*)"data_table", (char*)"sum(COUNT)", (char*)"", sqlResult);
		split_result = strtok_r(sqlResult, "#", &savePtr);
		totalCnt = atoi(split_result);
		if (totalCnt > dbConfig.iDataMaxCnt)
		{
			bCntMax = true;
			char cTName[15] = {0};
			char condition[128] = {0};
			//清空最头里那张表
			memset(sqlResult, 0, 56);
			memset(condition, 0, 128);
			sprintf(condition, "where length(tname)>4 and COUNT>0 order by tname");
			select_from_table_no_lock(SQLCTL_BACKUP_DB_PATH, (char*)"data_table", (char*)"TNAME", condition, sqlResult);
			split_result = strtok_r(sqlResult, "#", &savePtr);
			strcpy(cTName, split_result);
			//清空
			memset(sql, 0, sizeof(sql));
			sprintf(sql, formatDel, cTName);
			CPM_ExecSql(sql, NULL, NULL, true, 0);
			//更新data_table
			memset(sql, 0, sizeof(sql));
			sprintf(sql, formatUpdate, 0, "0", cTName);
			CPM_ExecSql(sql, NULL, NULL, true, 0);
		}
		else
		{
			bCntMax = false;
		}
	}
	while(bCntMax);
	
	return true;
}


bool CSqlCtl::InitializeTempDB()
{
	bool ret = false;
	CTblLockDb.lock();
	//ret = RecoverDB(SQLCTL_HISTORY_DB_PATH, SQLCTL_TEMP_DB_PATH, SQLCTL_SAMPLE_DB_PATH);
	ret = RecoverDB_New(SQLCTL_TEMP_DB_PATH);
	CTblLockDb.unlock();
	return ret;
}
bool CSqlCtl::InitializeCPMDB()
{
	bool ret = false;
	//CTblLockDb.lock();
	//ret = RecoverDB(SQLCTL_HISTORY_DB_PATH, SQLCTL_TEMP_DB_PATH, SQLCTL_SAMPLE_DB_PATH);
	ret = RecoverDB_New(SQLCTL_HISTORY_DB_PATH);
	//CTblLockDb.unlock();
	return ret;
}

/******************************************************************************/
/* Function Name  : RecoverDB_New                                                 */
/* Description    : 临时数据库初始化                                          */
/* Input          : const char *src       源数据库                            */
/*                  const char *dst       目的数据库						  */
/*                  const char *sample    模板数据库						  */
/* Output         : none                                                      */
/* Return         : true    操作成功                                          */
/*                  false   操作失败                                          */
/******************************************************************************/

bool CSqlCtl::RecoverDB_New(const char* dst)
{
	int ret = false;
	char syscmd[128] = {0};
	int retyr_cnt = 3;

	while(retyr_cnt-- > 0)
	{
		memset(syscmd, 0, sizeof(syscmd));
		sprintf(syscmd, "echo \".dump\"| sqlite3 %s | sqlite3 %s.temp", dst, dst);
		if(0 > system(syscmd))
		{
			printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
			continue;
		}

		printf("修复成功\n");
		memset(syscmd, 0, sizeof(syscmd));
		sprintf(syscmd, "mv %s.temp %s", dst, dst);
		if(0 > system(syscmd))
		{
			printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
			continue;
		}
		ret = true;
		break;
	}
	return ret;
}

/******************************************************************************/
/* Function Name  : RecoverDB                                                 */
/* Description    : 临时数据库初始化                                          */
/* Input          : const char *src       源数据库                            */
/*                  const char *dst       目的数据库						  */
/*                  const char *sample    模板数据库						  */
/* Output         : none                                                      */
/* Return         : true    操作成功                                          */
/*                  false   操作失败                                          */
/******************************************************************************/

bool CSqlCtl::RecoverDB(const char* src, const char* dst, const char* sample)
{
	int ret = false;
	char syscmd[128] = {0};
	int retyr_cnt = 3;

	while(retyr_cnt-- > 0)
	{
		//用模板数据库覆盖临时数据库
		memset(syscmd, 0, sizeof(syscmd));
		sprintf(syscmd, "cp -r %s %s", sample, dst);
		if(0 > system(syscmd))
		{
			printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
			continue;
		}

		//拷贝基础数据		
		if(!CopyTable(src, dst, "BLACK_LIST"))
			continue;
		if(!CopyTable(src, dst, "CALL"))
			continue;
		if(!CopyTable(src, dst, "CARD_MANAGER"))
			continue;
		if(!CopyTable(src, dst, "CORP_INFO"))
			continue;
		if(!CopyTable(src, dst, "DATA_TYPE"))
			continue;
		if(!CopyTable(src, dst, "DEVICE_CMD"))
			continue;
		if(!CopyTable(src, dst, "DEVICE_DETAIL"))
			continue;
		if(!CopyTable(src, dst, "DEVICE_INFO"))
			continue;
		if(!CopyTable(src, dst, "DEVICE_TYPE"))
			continue;
		if(!CopyTable(src, dst, "DOOR_LIMIT"))
			continue;
		if(!CopyTable(src, dst, "FP_INFO"))
			continue;
		if(!CopyTable(src, dst, "MANUFACTURER"))
			continue;
		if(!CopyTable(src, dst, "ROLE"))
			continue;
		if(!CopyTable(src, dst, "USER_INFO"))
			continue;
		ret = true;
		break;
	}
	return ret;
}

/******************************************************************************/
/* Function Name  : CopyTable                                                 */
/* Description    : 表拷贝													  */
/* Input          : const char *src       源数据库                            */
/*                  const char *dst       目的数据库						  */
/*                  const char *tablename 表名称						      */
/* Output         : none                                                      */
/* Return         : true    操作成功                                          */
/*                  false   操作失败                                          */
/******************************************************************************/
bool CSqlCtl::CopyTable(const char* src, const char* dst, const char* tablename)
{
	//bool ret = false;
	char syscmd[128] = {0};

	//导出表数据
	const char* export_format = "sqlite3 %s \"select * from %s\" > %s/%s.dat";
	sprintf(syscmd, export_format, src, tablename, DB_DIR, tablename);
	printf("%s\n", syscmd);
	if(0 > system(syscmd))
	{
		printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
		return false;
	}

	//导入表数据
	const char* import_format = "sqlite3 %s \".import %s/%s.dat %s\"";
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, import_format, dst, DB_DIR, tablename, tablename);
	printf("%s\n", syscmd);
	if(0 > system(syscmd))
	{
		printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
		return false;
	}

	//删除临时导出文件
	const char* delete_f = "rm -rf %s/%s.dat";
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, delete_f, DB_DIR, tablename);
	printf("%s\n", syscmd);
	if(0 > system(syscmd))
	{
		printf("cmd: %s\t error: %s\n", syscmd, strerror(errno)); 
		return false;
	}
	return true;
}



/******************************************************************************/
/* Function Name  : insert_into_table                                         */
/* Description    : 将数据插入表中                                            */
/* Input          : char *table     目标表名称                                */
/*                  char *record    准备插入的记录(直接被放到values后)        */
/* Output         : none                                                      */
/* Return         : true    操作成功                                          */
/*                  false   操作失败                                          */
/******************************************************************************/
bool CSqlCtl::insert_into_table( const char* table, char* record, bool bRedo)
{
	bool ret = false;
	bool errorType = false;
    char* errmsg = 0;
     char insert[2048]="insert into \"";
     const char *section=" values(";
     const char *semi=");";
     strcat(insert,table);
     strcat(insert,"\"");
     strcat(insert,section);
     strcat(insert,record);
     strcat(insert,semi);
     //sqlite3_busy_timeout(db, 2000);

   //  printf("SQL executing:db[%s] [%s]\n", m_dbPath,insert);
	 
	 CTblLockDb.lock();
	 sqlite3* db = 0;
	 if (SQLITE_OK == sqlite3_open(m_dbPath, &db))
	 {
		 
		 sqlite3_busy_timeout(db, 200);		
		 DBG_DB(("insert[%s]\n", insert));
		 int dbResult = SYS_STATUS_FAILED;
		 do
		 {
			 if(SQLITE_OK == sqlite3_exec(db,insert,0,0,&errmsg))
			 {
				 dbResult = SYS_STATUS_SUCCESS;
				 ret = true;
			 }		
			 else
			 {
				 printf("DB ERROR:%s, %s\n",errmsg, insert);
				 if (NULL != strstr(errmsg, "database is locked"))//数据库被锁
				 {
					 dbResult = SYS_STATUS_DB_LOCKED_ERROR;
				 }
				 errorType = error(errmsg);
			 }
			 sqlite3_free(errmsg);
		 }while(SYS_STATUS_DB_LOCKED_ERROR == dbResult);
		 
	 }
	 else
	 {
		 printf("DB open failed\n");
	 }
	 int closeret;
	 if(SQLITE_OK != (closeret = sqlite3_close(db)))
		 printf("DB sqlite3_close failed ret[%d]\n", closeret);
	 CTblLockDb.unlock();
	 if (errorType)
	 {
		 InitializeTempDB();
	 }
        return ret;
}

/******************************************************************************/
/* Function Name  : update_into_table                                         */
/* Description    : 将数据插入表中                                            */
/* Input          : char *table     目标表名称                                */
/*                  char *record    更新的记录                                */
/* Output         : none                                                      */
/* Return         : true    操作成功                                          */
/*                  false   操作失败                                          */
/******************************************************************************/
bool CSqlCtl::update_into_table(const char *table,char *record, bool bRedo)
{
	bool ret = false;
    char* errmsg = NULL;
	bool errorType = false;
     char update[2048]="update ";
     const char *section=" set ";
     strcat(update,table);
     strcat(update,section);
     strcat(update,record);
     strcat(update,";");
 
	 CTblLockDb.lock(); 
	 sqlite3* db = NULL;
	 
	 if (SQLITE_OK == sqlite3_open(m_dbPath, &db))
	 {
		 printf("update[%s]\n", update);
		 
		 sqlite3_busy_timeout(db, 200);
		 int dbResult = SYS_STATUS_FAILED;
		 do
		 {
			 if(SQLITE_OK == sqlite3_exec(db,update,0,0,&errmsg))
			 {
				 dbResult = SYS_STATUS_SUCCESS;
				 ret = true;
			 }
			 else
			 {
				 printf("DB ERROR:%s, %s\n",errmsg, update);
				 errorType = error(errmsg);
				 if (NULL != strstr(errmsg, "database is locked"))//数据库被锁
				 {
					 dbResult = SYS_STATUS_DB_LOCKED_ERROR;
				 }	
			 }
			 sqlite3_free(errmsg);	
		}while(SYS_STATUS_DB_LOCKED_ERROR == dbResult);
	 }
	// if(NULL != db)
	 {
		 sqlite3_close(db);
	 }
	 db = NULL;
	 CTblLockDb.unlock();
 
	 if (errorType)
	 {
		 InitializeTempDB();
	 }
     return ret;
}

/******************************************************************************/
/* Function Name  : select_from_table                                         */
/* Description    : 从表中查出数据                                            */
/* Input          : char *table     数据表名称                                */
/*                  char *column    需要查出的数据列                          */
/*                  char *condition 查询条件                                  */
/* Output         : char *result    查询出来的记录(数据间以#隔开)             */
/* Return         : true    操作成功                                          */
/*                  false   操作失败                                          */
/******************************************************************************/
int CSqlCtl::select_from_table(const char* dbName, char *table, char *column, char *condition, char *result)
{
	bool errorType = false;
    char* errmsg = NULL;
	int ret = -1;
    char select[256] = "select ";                  //select
    strcat(select, column);                        //select *
    strcat(select, " from ");                      //select * from 
    strcat(select, table);                         //select * from table
    strcat(select, " ");
    strcat(select, condition);                     //select * from table where...
    strcat(select, ";");
	CTblLockDb.lock(); 
	sqlite3* db = NULL;
	if (SQLITE_OK == sqlite3_open(dbName, &db))
	{
		sqlite3_busy_timeout(db, 200);
		int dbResult = SYS_STATUS_FAILED;
		do
		{
			if(SQLITE_OK == sqlite3_exec(db, select, loadData, result, &errmsg))
			{
				ret = 0;
				dbResult = SYS_STATUS_SUCCESS;
			}
			else
			{
				errorType = error(errmsg);
				DBG_DB(("DB ERROR:%s, %s\n",errmsg, select));
				if (NULL != strstr(errmsg, "database is locked"))
				{
					ret = 1;
					dbResult = SYS_STATUS_DB_LOCKED_ERROR;
				}
				
			}
			sqlite3_free(errmsg);
		}while(SYS_STATUS_DB_LOCKED_ERROR == dbResult);
	}
	//if(NULL != db)
	{
		sqlite3_close(db);
	}
	db = NULL;
	CTblLockDb.unlock();

	if (errorType)
	{
		InitializeTempDB();
	}
    return ret;
}
/******************************************************************************/
/* Function Name  : select_from_table_no_lock                                         */
/* Description    : 从表中查出数据                                            */
/* Input          : char *table     数据表名称                                */
/*                  char *column    需要查出的数据列                          */
/*                  char *condition 查询条件                                  */
/* Output         : char *result    查询出来的记录(数据间以#隔开)             */
/* Return         : true    操作成功                                          */
/*                  false   操作失败                                          */
/******************************************************************************/
int CSqlCtl::select_from_table_no_lock(const char* dbName, char *table, char *column, char *condition, char *result)
{
	bool errorType = false;
	char* errmsg = NULL;
	int ret = -1;
	char select[256] = "select ";                  //select
	strcat(select, column);                        //select *
	strcat(select, " from ");                      //select * from 
	strcat(select, table);                         //select * from table
	strcat(select, " ");
	strcat(select, condition);                     //select * from table where...
	strcat(select, ";");
	printf("select_from_table_no_lock[%s]\n", select);
//	CTblLockDb.lock(); 
	sqlite3* db = NULL;
	if (SQLITE_OK == sqlite3_open(dbName, &db))
	{
		sqlite3_busy_timeout(db, 200);
		int dbResult = SYS_STATUS_FAILED;
		do
		{
			if(SQLITE_OK == sqlite3_exec(db, select, loadData, result, &errmsg))
			{
				ret = 0;
				dbResult = SYS_STATUS_SUCCESS;
				printf("success[%s]\n", result);
			}
			else
			{
				errorType = error(errmsg);
				DBG_DB(("DB ERROR:%s, %s\n",errmsg, select));
				if (NULL != strstr(errmsg, "database is locked"))
				{
					ret = 1;
					dbResult = SYS_STATUS_DB_LOCKED_ERROR;
				}

			}
			sqlite3_free(errmsg);
		}while(SYS_STATUS_DB_LOCKED_ERROR == dbResult);
	}
	//if(NULL != db)
	{
		sqlite3_close(db);
	}
	db = NULL;
//	CTblLockDb.unlock();
	
	if (errorType)
	{
		printf("select_from_table_no_lock errorType[%d]\n", errorType);
		InitializeCPMDB();
	}
	return ret;
}
int CSqlCtl::loadData(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{
    for(int i=0; i<n_column; i++)
    {
		if (NULL == column_value[i])
		{
			strcat((char *)data, "#");
			continue;
		}

        strcat((char *)data, column_value[i]);

        if( strlen(column_value[i]) == 0 )
        {
            strcat((char *)data, " #");
        }
        else
		{
            strcat((char *)data, "#");
        }

    }

    return 0;
}


/******************************************************************************/
/* Function Name  : delete_record                                             */
/* Description    : 从表中删除记录                                            */
/* Input          : char *table         数据表名称                            */
/*                  char *expression    要删除的数据满足的条件                */
/* Output         : none                                                      */
/* Return         : true    操作成功                                          */
/*                  false   操作失败                                          */
/******************************************************************************/
bool CSqlCtl::delete_record(const char *table,char *expression, bool bRedo)
{
	bool errorType = false;
	bool ret = false;
    char s_delete[512]="delete from ";
    char where[256]=" ";
	char* errmsg = NULL;
    strcat(s_delete,table);
    strcat(where,expression);
    strcat(where,";");
    if(NULL!=expression)
    {
        strcat(s_delete,where);
    }

	CTblLockDb.lock();
	sqlite3* db = NULL;
	if (SQLITE_OK == sqlite3_open(m_dbPath, &db))
	{
		sqlite3_busy_timeout(db, 200);
		
		
		printf("delete: %s\n", s_delete);
		int dbResult = SYS_STATUS_FAILED;
		do
		{
			if(SQLITE_OK == sqlite3_exec(db,s_delete,0,0,&errmsg))
			{
				dbResult = SYS_STATUS_SUCCESS;
				ret = true;
			}
			else
			{
				errorType = error(errmsg);
				DBG_DB(("DB ERROR:%s, %s\n",errmsg, s_delete));
				if (NULL != strstr(errmsg, "database is locked"))//数据库被锁
				{
					dbResult = SYS_STATUS_DB_LOCKED_ERROR;
				}
			}
			sqlite3_free(errmsg);		
		}while(SYS_STATUS_DB_LOCKED_ERROR == dbResult);
		
	}
	//if(NULL != db)
	{
		sqlite3_close(db);
	}
	db = NULL;
	CTblLockDb.unlock();

	if (errorType)
	{
		InitializeTempDB();
	}
	return ret;
}

bool CSqlCtl::close_database()
{
	bool ret = false;
	/*
	CTblLockDb.lock();
	if (NULL != db)
	{
		if(SQLITE_OK == sqlite3_close(db))
		{
			db = NULL;
			ret = true;
		}
	}
	CTblLockDb.unlock();
	*/
   return ret;
}


/******************************************************************************/
/* Function Name  : select_from_table                                         */
/* Description    : 从表中查出数据                                            */
/* Input          : char *table     数据表名称                                */
/*                  char *column    需要查出的数据列                          */
/*                  char *condition 查询条件                                  */
/* Output         : char *result    查询出来的记录(数据间以#隔开)             */
/* Return         : true    操作成功                                          */
/*                  false   操作失败                                          */
/******************************************************************************/
int CSqlCtl::select_from_tableEx(char *sql, sqlite3_callback FuncSelectCallBack, void * userData)
{
	bool errorType = false;
	int ret = -1;
	char* errmsg = NULL;
	CTblLockDb.lock();
	sqlite3* db = NULL;
	if (SQLITE_OK == sqlite3_open(m_dbPath, &db))
	{
		sqlite3_busy_timeout(db, 200);
		int dbResult = SYS_STATUS_FAILED;
		do
		{
			if(SQLITE_OK == sqlite3_exec(db, sql, FuncSelectCallBack, userData, &errmsg))
			{
				dbResult = SYS_STATUS_SUCCESS;
				ret = 0;
			}
			else
			{
				errorType = error(errmsg);
				DBG(("DB ERROR:%s, %s\n",errmsg, sql));
				if (NULL != strstr(errmsg, "database is locked"))
				{
					dbResult = SYS_STATUS_DB_LOCKED_ERROR;
					ret = 1;
				}
			}
			sqlite3_free(errmsg);
		}while(SYS_STATUS_DB_LOCKED_ERROR == dbResult);
	}
	//if(NULL != db)
	{
		sqlite3_close(db);
	}
	db = NULL;
	CTblLockDb.unlock();

	if (errorType)
	{
		InitializeTempDB();
	}
	return ret;
}

/******************************************************************************/
/* Function Name  : ExecSql                                         */
/* Description    : 从表中查出数据                                            */
/* Input          : char *table     数据表名称                                */
/*                  char *column    需要查出的数据列                          */
/*                  char *condition 查询条件                                  */
/* Output         : char *result    查询出来的记录(数据间以#隔开)             */
/* Return         : true    操作成功                                          */
/*                  false   操作失败                                          */
/******************************************************************************/
bool CSqlCtl::ExecSql(const char *sql, sqlite3_callback FuncSelectCallBack, void * userData, bool bRedo, time_t tTime)
{
	bool errorType = false;
	bool ret = false;
	char* errmsg = NULL;
	CTblLockDb.lock();
	sqlite3* db = NULL;
	if (SQLITE_OK == sqlite3_open(m_dbPath, &db))
	{
		sqlite3_busy_timeout(db, 200);
		printf("ExecSql--------sql[%s]\n", sql);
		
		int dbResult = SYS_STATUS_FAILED;
		do
		{
			if(SQLITE_OK == sqlite3_exec(db, sql, FuncSelectCallBack, userData, &errmsg))
			{
				dbResult = SYS_STATUS_SUCCESS;
				ret = true;
			}
			else
			{
				errorType = error(errmsg);
				if (NULL != strstr(errmsg, "database is locked"))//数据库被锁
				{
					dbResult = SYS_STATUS_DB_LOCKED_ERROR;
				}
				DBG_DB(("DB ERROR:%s, %s\n",errmsg, sql));
			}
			sqlite3_free(errmsg);
		}while(SYS_STATUS_DB_LOCKED_ERROR == dbResult);
		
	}
	//if(NULL != db)
	{
		sqlite3_close(db);
	}
	db = NULL;
	CTblLockDb.unlock();

	if (errorType)
	{
		InitializeTempDB();
	}
	return ret;
}

/******************************************************************************/
/* Function Name  : CPM_ExecSql                                         */
/* Description    : 从表中查出数据                                            */
/* Input          : char *table     数据表名称                                */
/*                  char *column    需要查出的数据列                          */
/*                  char *condition 查询条件                                  */
/* Output         : char *result    查询出来的记录(数据间以#隔开)             */
/* Return         : true    操作成功                                          */
/*                  false   操作失败                                          */
/******************************************************************************/
bool CSqlCtl::CPM_ExecSql(const char *sql, sqlite3_callback FuncSelectCallBack, void * userData, bool bRedo, time_t tTime)
{
	bool errorType = false;
	bool ret = false;
	char* errmsg = NULL;
//	CTblLockDb.lock();
	sqlite3* db = NULL;
//	printf("CPM_ExecSql\n");
	if (SQLITE_OK == sqlite3_open(SQLCTL_BACKUP_DB_PATH, &db))
	{
		sqlite3_busy_timeout(db, 200);
		printf("CPM_ExecSql-------sql[%s]\n", sql);

		int dbResult = SYS_STATUS_FAILED;
		do
		{
			if(SQLITE_OK == sqlite3_exec(db, sql, FuncSelectCallBack, userData, &errmsg))
			{
				dbResult = SYS_STATUS_SUCCESS;
				ret = true;
//				printf("CPM_ExecSql SQLITE_OK\n");
			}
			else
			{
				errorType = error(errmsg);
				if (NULL != strstr(errmsg, "database is locked"))//数据库被锁
				{
					dbResult = SYS_STATUS_DB_LOCKED_ERROR;
				}
				DBG_DB(("DB ERROR:%s, %s\n",errmsg, sql));
			}
			sqlite3_free(errmsg);
//			printf("CPM_ExecSql sqlite3_free\n");
		}while(SYS_STATUS_DB_LOCKED_ERROR == dbResult);

	}
//	printf("CPM_ExecSql sqlite3_close\n");
	//if(NULL != db)
	{
		sqlite3_close(db);
	}
	
	db = NULL;
//	CTblLockDb.unlock();

	if (errorType)
	{
		printf("CPM_ExecSql errorType[%d]\n", errorType);
		InitializeCPMDB();
	}
//	printf("CPM_ExecSql ret[%d]\n", ret);
	return ret;
}

/************************************************************************/
/* Application Functions                                                */
/************************************************************************/
typedef struct _ATTR_CALLBACK_NODE
{
	char sn[DEVICE_SN_LEN_TEMP];
	char attr_name[21];
	char s_define[129];
	char private_attr[256];
}ATTR_CALLBACK_NODE;
typedef struct _DEVICE_ATTR_CALLBACK_DATA
{
	int iCount;
	ATTR_CALLBACK_NODE szAttrNode[128];
}DEVICE_ATTR_CALLBACK_DATA;

typedef struct _CMD_CALLBACK_NODE
{
	char sn[DEVICE_ACTION_SN_LEN_TEMP];
	char act_name[21];
}CMD_CALLBACK_NODE;
typedef struct _DEVICE_CMD_CALLBACK_DATA
{
	int iCount;
	CMD_CALLBACK_NODE stCmdNode[128];
}DEVICE_CMD_CALLBACK_DATA;
/******************************************************************************/
/* Function Name  : select_from_table                                         */
/* Description    : 从表中查出数据                                            */
/* Input          : char *table     数据表名称                                */
/*                  char *column    需要查出的数据列                          */
/*                  char *condition 查询条件                                  */
/* Output         : char *result    查询出来的记录(数据间以#隔开)             */
/* Return         : true    操作成功                                          */
/*                  false   操作失败                                          */
/******************************************************************************/
bool CSqlCtl::AppAddDeviceDetail(char* id, char* cname, char* upper, char* upper_channel, char* ctype, char* self_id, char status, char* private_attr, char* brief, char* reserve2, char* memo)
{
	//检查总属性条数是否超标lsd
	char queryResult[512] = {0};
	if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"device_roll", (char*)"count(*)", (char*)"", queryResult))
	{
		return false;
	}
	char* split_result = NULL;
	char* savePtr = NULL;
	split_result = strtok_r(queryResult, "#", &savePtr);
	if (atoi(split_result)  > g_RecordCntCfg.Attr_Max)
	{
		return false;
	}


	
	const char* sqlFormat = "insert into device_detail(id, cname, upper, upper_channel, ctype, self_id, status, private_attr, brief, preset, memo) "
		"values('%s', '%s', '%s', '%s', '%s', '%s', %c, '%s','%s', '%s', '%s');";
	char sql[512] = {0};
	sprintf(sql, sqlFormat, id, cname, upper, upper_channel, ctype, self_id, status, private_attr, brief, reserve2, memo);

	bool ret = false;
	char* errmsg = NULL;
	CTblLockDb.lock();
	sqlite3* db = NULL;
	if (SQLITE_OK == sqlite3_open(m_dbPath, &db))
	{
		sqlite3_busy_timeout(db, 2000);
		DBG_DB(("sql[%s]\n", sql));
		do 
		{
			//插入到设备表
			int dbResult = SYS_STATUS_FAILED;
			do
			{
				if(SQLITE_OK == sqlite3_exec(db, sql, NULL, NULL, &errmsg))
				{
					dbResult = SYS_STATUS_SUCCESS;
				}
				else
				{
					if (NULL != strstr(errmsg, "database is locked"))//数据库被锁
					{
						dbResult = SYS_STATUS_DB_LOCKED_ERROR;
					}
					DBG_DB(("DB ERROR:%s, %s\n",errmsg, sql));
				}
				sqlite3_free(errmsg);
			}while(SYS_STATUS_DB_LOCKED_ERROR == dbResult);
			if(SYS_STATUS_SUCCESS != dbResult)
				break;
			dbResult = SYS_STATUS_FAILED;

			//插入相关采集属性到device_roll表 lhy V10.0.0.18
			sqlFormat = "select id, sn, attr_name, s_define, private_attr from device_attr where id=substr('%s', 1, %d);";
			memset(sql, 0, sizeof(sql));
			sprintf(sql, sqlFormat, id, DEVICE_TYPE_LEN);
			DBG_DB(("sql[%s]\n", sql));
			DEVICE_ATTR_CALLBACK_DATA stAttrData;
			memset((unsigned char*)&stAttrData, 0, sizeof(DEVICE_ATTR_CALLBACK_DATA));
			do
			{
				if(SQLITE_OK == sqlite3_exec(db, sql, CSqlCtl::SelectFromDeviceAttrCallBack, (void*)&stAttrData, &errmsg))
				{
					dbResult = SYS_STATUS_SUCCESS;
				}
				else
				{					
					DBG_DB(("DB ERROR:%s, %s\n",errmsg, sql));
					if (NULL != strstr(errmsg, "database is locked"))//数据库被锁
					{
						dbResult = SYS_STATUS_DB_LOCKED_ERROR;
					}
				}
				sqlite3_free(errmsg);
			}while(SYS_STATUS_DB_LOCKED_ERROR == dbResult);
			if(SYS_STATUS_SUCCESS != dbResult)
				break;
			dbResult = SYS_STATUS_FAILED;

			DBG_DB(("sql[%s] stAttrData Count[%d]\n", sql, stAttrData.iCount));
			for(int i=0; i<stAttrData.iCount; i++)
			{
				//插入到device_roll表
				memset(sql, 0, sizeof(sql));
				const char* sqlFormat = "insert into device_roll(id, sn, attr_name, s_define, private_attr, standard, defence, sta_show, status, v_id, v_sn) "
					"values('%s', '%s', '%s', '%s', '%s', '', '', '0','0', '', '');";
				sprintf(sql, sqlFormat, id, stAttrData.szAttrNode[i].sn, stAttrData.szAttrNode[i].attr_name, stAttrData.szAttrNode[i].s_define, stAttrData.szAttrNode[i].private_attr);
				DBG_DB(("sql[%s]\n", sql));
				do
				{
					if(SQLITE_OK == sqlite3_exec(db, sql, NULL, NULL, &errmsg))
					{
						dbResult = SYS_STATUS_SUCCESS;
					}
					else
					{					
						DBG_DB(("DB ERROR:%s, %s\n",errmsg, sql));
						if (NULL != strstr(errmsg, "database is locked"))//数据库被锁
						{
							dbResult = SYS_STATUS_DB_LOCKED_ERROR;
						}
					}
					sqlite3_free(errmsg);
				}while(SYS_STATUS_DB_LOCKED_ERROR == dbResult);
				if(SYS_STATUS_SUCCESS != dbResult)
					break;
				dbResult = SYS_STATUS_FAILED;
			}

			//插入相关动作属性到device_act表 V10.0.0.18		
			sqlFormat = "select id, sn, cmdname from device_cmd where id=substr('%s', 1, %d);";
			memset(sql, 0, sizeof(sql));
			sprintf(sql, sqlFormat, id, DEVICE_TYPE_LEN);
			DBG_DB(("sql[%s]\n", sql));
			DEVICE_CMD_CALLBACK_DATA stCmdData;
			memset((unsigned char*)&stCmdData, 0, sizeof(DEVICE_CMD_CALLBACK_DATA));
			do
			{
				if(SQLITE_OK == sqlite3_exec(db, sql, CSqlCtl::SelectFromDeviceCmdCallBack, (void*)&stCmdData, &errmsg))
				{
					dbResult = SYS_STATUS_SUCCESS;
				}
				else
				{					
					DBG_DB(("DB ERROR:%s, %s\n",errmsg, sql));
					if (NULL != strstr(errmsg, "database is locked"))//数据库被锁
					{
						dbResult = SYS_STATUS_DB_LOCKED_ERROR;
					}
				}
				sqlite3_free(errmsg);
			}while(SYS_STATUS_DB_LOCKED_ERROR == dbResult);			
			if(SYS_STATUS_SUCCESS != dbResult)
				break;
			dbResult = SYS_STATUS_FAILED;
	
			DBG_DB(("sql[%s] stCmdDataCount[%d]\n", sql, stCmdData.iCount));
			for(int i=0; i<stCmdData.iCount; i++)
			{
				//插入到device_act表
				memset(sql, 0, sizeof(sql));
				const char* sqlFormat = "insert into device_act(id, sn, cmdname, status) "
					"values('%s', '%s', '%s', '1');";
				sprintf(sql, sqlFormat, id, stCmdData.stCmdNode[i].sn, stCmdData.stCmdNode[i].act_name);

				DBG_DB(("sql[%s]\n", sql));
				do
				{
					if(SQLITE_OK == sqlite3_exec(db, sql, NULL, NULL, &errmsg))
					{
						dbResult = SYS_STATUS_SUCCESS;
					}
					else
					{					
						DBG_DB(("DB ERROR:%s, %s\n",errmsg, sql));
						if (NULL != strstr(errmsg, "database is locked"))//数据库被锁
						{
							dbResult = SYS_STATUS_DB_LOCKED_ERROR;
						}
					}
					sqlite3_free(errmsg);
				}while(SYS_STATUS_DB_LOCKED_ERROR == dbResult);			
				if(SYS_STATUS_SUCCESS != dbResult)
					break;
				dbResult = SYS_STATUS_FAILED;
			}
			ret = true;
		} while (false);
	}
	
	//if(NULL != db)
	{
		sqlite3_close(db);
		db = NULL;
	}
	CTblLockDb.unlock();

	return ret;
}


int CSqlCtl::SelectFromDeviceAttrCallBack(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{
	int ret = SQLITE_OK;
	DEVICE_ATTR_CALLBACK_DATA* pStAttrData = (DEVICE_ATTR_CALLBACK_DATA*)data;

	//生成device_roll结点
	strncpy(pStAttrData->szAttrNode[pStAttrData->iCount].sn, column_value[1], DEVICE_SN_LEN);
	strcpy(pStAttrData->szAttrNode[pStAttrData->iCount].attr_name, column_value[2]);
	strcpy(pStAttrData->szAttrNode[pStAttrData->iCount].s_define, column_value[3]);
	GetDefaultPrivateAttr(column_value[4], pStAttrData->szAttrNode[pStAttrData->iCount].private_attr);
	pStAttrData->iCount++;

	return ret;
}

int CSqlCtl::GetDefaultPrivateAttr(char* attrDefine, char* private_attr)    //采集属性私有参数初始话
{
	int ret = SYS_STATUS_FAILED;
	if (NULL != strstr(attrDefine, "interval"))
	{
		strcat(private_attr, "interval=60000,");
	}
	return ret;
}


int CSqlCtl::SelectFromDeviceCmdCallBack(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{

	int ret = SQLITE_OK;
	DEVICE_CMD_CALLBACK_DATA* pStCmdData = (DEVICE_CMD_CALLBACK_DATA*)data;

	//生成device_roll结点
	strncpy(pStCmdData->stCmdNode[pStCmdData->iCount].sn, column_value[1], DEVICE_ACTION_SN_LEN);
	strcpy(pStCmdData->stCmdNode[pStCmdData->iCount].act_name, column_value[2]);
	pStCmdData->iCount++;
	return ret;
}

/******************************************************************************/
/* Function Name  : DeleteFromAlertNowTable                                  */
/* Description    : 删除告警表老数据                                          */
/* Input          : int   vType          数值类型                             */
/* Output         : none                                                      */
/* Return         : none				                        			  */
/******************************************************************************/
void CSqlCtl::DeleteFromAlertNowTable(char* deviceId, char* attrId, int nRecordeCount)
{
	char szDeviceId[DEVICE_ID_LEN_TEMP] = {0};

	memset(szDeviceId, 0, sizeof(szDeviceId));

	memcpy(szDeviceId, deviceId, DEVICE_ID_LEN);

	char szCondition[512] = {0};
	if (0 < nRecordeCount)
	{
		sprintf(szCondition, "where id = '%s' and attr_id = '%s' and sn not in (select sn from alert_now where id = '%s' order by sn desc limit 0, %d)", szDeviceId, attrId, szDeviceId, nRecordeCount);
	}
	else
	{
		sprintf(szCondition, "where id = '%s' and attr_Id = '%s'", szDeviceId, attrId);
	}
	g_SqlCtl.delete_record("alert_now", szCondition, false);
}


/******************************************************************************/
/* Function Name  : UpdateAlertInfoTable                                            */
/* Description    : 设备状态变更                                            */
/* Input          : int   vType          数值类型                             */
/* Output         : none                                                      */
/* Return         : true       数据在正常范围内                               */
/*                  false      数据在正常范围外                               */
/******************************************************************************/
void CSqlCtl::UpdateAlertInfoTable(char* deviceId, char* attrId, char status, char* time)
{
	char sql[512] = {0};
	//更新到告警表Alert_Info表
	{
		const char* sqlFormat = "update alert_info set status = '%c', ETIME='%s' where id = '%s' and attr_Id = '%s' and status = '0';";
		sprintf(sql, sqlFormat, status, time, deviceId, attrId);
	}

	g_SqlCtl.ExecSql(sql, NULL, NULL, true, 0);
}

/******************************************************************************/
/* Function Name  : InsertAlertInfoTable                                      */
/* Description    : 设备状态变更                                            */
/* Input          : int   vType          数值类型                             */
/* Output         : none                                                      */
/* Return         : true       数据在正常范围内                               */
/*                  false      数据在正常范围外                               */
/******************************************************************************/
void CSqlCtl::InsertAlertInfoTable(char* deviceId, char* attrId, char* time, int alarmLevel , char* alarmDescribe, char status, char* level)
{
	const char* sqlFormat = "insert into %s(sn, id, Attr_Id, ctime, ctype, value, status, level) "
		"values(NULL, '%s', '%s', '%s', '%d', '%s', '%c', '%s');";

	char sql[512] = {0};
	//插入到alert_info表
	sprintf(sql, sqlFormat, 
		"alert_info",
		deviceId, 
		attrId,
		time, 
		alarmLevel, 
		alarmDescribe, 
		status,
		level);
	g_SqlCtl.ExecSql(sql, NULL, NULL, true, 0);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, sqlFormat, 
		"alert_now",
		deviceId, 
		attrId,
		time, 
		alarmLevel, 
		alarmDescribe, 
		status,
		level);
	g_SqlCtl.ExecSql(sql, NULL, NULL, false, 0);
}

/******************************************************************************/
/* Function Name  : GetUserInfoByCardId                                      */
/* Description    : 设备状态变更                                            */
/* Input          : int   vType          数值类型                             */
/* Output         : none                                                      */
/* Return         : true       数据在正常范围内                               */
/*                  false      数据在正常范围外                               */
/******************************************************************************/
int CSqlCtl::GetUserInfoByCardId(char* cardId, char* szUserId, char* szUserName)
{
	int ret = SYS_STATUS_FAILED;
	char sql[512] = {0};

	//判断设备是否存在
	const char* sqlFormat = "where a.sys_id = b.id and b.ic = '%s'";
	sprintf(sql, sqlFormat, cardId);
	do
	{
		char queryResult[128] = {0};
		if(0 != select_from_table(DB_TODAY_PATH, (char*)"user_info a, card_manager b ", (char*)"b.id, a.cname", sql, queryResult))
		{
			break;
		}

		char* split_result = NULL;
		char* savePtr = NULL;
		split_result = strtok_r(queryResult, "#", &savePtr);
		if (NULL == split_result)
			break;
		strcpy(szUserId, split_result);
		split_result = strtok_r(NULL, "#", &savePtr);
		if(NULL == split_result)
			break;
		strcpy(szUserName, split_result);
		ret = SYS_STATUS_SUCCESS;
	}while(false);
	return ret;
}

void CSqlCtl::getModeConfig(MODE_CFG& modeConfig)
{
	memset((unsigned char*)&modeConfig, 0, sizeof(MODE_CFG));

	//生成统计类数据平均值表
	const char* format = "select mode_set.id, mode_set.status"
		" from mode_set; ";
	
	//查询数据库， 获取现有数据表中的设备属性信息
	if(0 != select_from_tableEx((char*)format, CSqlCtl::GetModeCfgCallBack, (void *)&modeConfig))
	{
		modeConfig.Ad = 1;
		modeConfig.GSM = 1;
		modeConfig.NetTerminal = 0;
		modeConfig.RS485_1 = 1;
		modeConfig.RS485_2 = 1;
		modeConfig.RS485_3 = 1;
		modeConfig.RS485_4 = 1;
		modeConfig.RS485_5 = 0;
		modeConfig.RS485_6 = 0;
		modeConfig.RS485_7 = 0;
		modeConfig.RS485_8 = 0;
		modeConfig.DB_Insert = 1;
		modeConfig.Transparent = 0;
	}	
}

int CSqlCtl::GetModeCfgCallBack(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{
	PMODE_CFG pStModeCfg = (PMODE_CFG)data;
	if('1' == column_value[1][0])
	{
		switch(atoi(column_value[0]))
		{
		case MOD_AD:
			pStModeCfg->Ad = 1;
			break;
		case MOD_GSM:
			pStModeCfg->GSM = 1;
			break;
		case MOD_NetTerminal:
			pStModeCfg->NetTerminal = 1;
			break;
		case MOD_TRANSPARENT:
			pStModeCfg->Transparent = 1;
			break;
		case MOD_RS485_1:
			pStModeCfg->RS485_1 = 1;
			break;
		case MOD_RS485_2:
			pStModeCfg->RS485_2 = 1;
			break;
		case MOD_RS485_3:
			pStModeCfg->RS485_3 = 1;
			break;
		case MOD_RS485_4:
			pStModeCfg->RS485_4 = 1;
			break;
		case MOD_RS485_5:
			pStModeCfg->RS485_5 = 1;
			break;
		case MOD_RS485_6:
			pStModeCfg->RS485_6 = 1;
			break;
		case MOD_RS485_7:
			pStModeCfg->RS485_7 = 1;
			break;
		case MOD_RS485_8:
			pStModeCfg->RS485_8 = 1;
			break;
		case MOD_DB_Insert:
			pStModeCfg->DB_Insert = 1;
			break;
		}
	}
	printf("GetModeCfgCallBack  mod[%d][%c]\n", atoi(column_value[0]), column_value[1][0]);
	return 0;
}



void CSqlCtl::getRecordCountConfig(RECORD_COUNT_CFG& recordConfig)
{
	memset((unsigned char*)&recordConfig, 0, sizeof(RECORD_COUNT_CFG));

	//生成统计类数据平均值表
	const char* format = "select config.MaxRecodeCnt01, "
		" config.MaxRecodeCnt02, "
		" config.MaxRecodeCnt03, "
		" config.MaxRecodeCnt04, "
		" config.MaxRecodeCnt05, "
		" config.MaxRecodeCnt06, "
		" config.MaxRecodeCnt07, "
		" config.MaxRecodeCnt08, "
		" config.MaxRecodeCnt09, "
		" config.MaxRecodeCnt10, "
		" config.MaxRecodeCnt11, "
		" config.MaxRecodeCnt12, "
		" config.MaxRecodeCnt13, "
		" config.MaxRecodeCnt14, "
		" config.MaxRecodeCnt15, "
		" config.MaxRecodeCnt16, "
		" config.MaxRecodeCnt17, "
		" config.MaxRecodeCnt18, "
		" config.MaxRecodeCnt19, "
		" config.MaxRecodeCnt20, "
		" config.MaxRecodeCnt21 "
		" from config; ";

	//查询数据库， 获取现有数据表中的设备属性信息
	if(0 != select_from_tableEx((char*)format, CSqlCtl::GetRecordCntCfgCallBack, (void *)&recordConfig))
	{
		DBG(("g_SqlCtl.getRecordCountConfig(&g_RecordCntCfg); Failed!"));
	}	

}

int CSqlCtl::GetRecordCntCfgCallBack(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{
	PRECORD_COUNT_CFG pStRecordCfg = (PRECORD_COUNT_CFG)data;
	
	pStRecordCfg->User_Max = atoi(column_value[0]);
	pStRecordCfg->Dev_Max = atoi(column_value[1]);
	pStRecordCfg->Attr_Max = atoi(column_value[2]);
	pStRecordCfg->Agent_Max = atoi(column_value[3]);
	pStRecordCfg->TimeCtrl_Max = atoi(column_value[4]);
	pStRecordCfg->Single_Agent_Max = atoi(column_value[5]);
	pStRecordCfg->NetSend_Max = atoi(column_value[6]);
	pStRecordCfg->NetRecv_Max = atoi(column_value[7]);
	pStRecordCfg->NetClientSend_Max = atoi(column_value[8]);
	pStRecordCfg->NetClientRecv_Max = atoi(column_value[9]);
	pStRecordCfg->RS485_1_Max = atoi(column_value[10]);
	pStRecordCfg->RS485_2_Max = atoi(column_value[11]);
	pStRecordCfg->RS485_3_Max = atoi(column_value[12]);
	pStRecordCfg->RS485_4_Max = atoi(column_value[13]);
	pStRecordCfg->RS485_5_Max = atoi(column_value[14]);
	pStRecordCfg->RS485_6_Max = atoi(column_value[15]);
	pStRecordCfg->RS485_7_Max = atoi(column_value[16]);
	pStRecordCfg->RS485_8_Max = atoi(column_value[17]);
	pStRecordCfg->ADS_Max = atoi(column_value[18]);
	pStRecordCfg->GSM_Max = atoi(column_value[19]);
	pStRecordCfg->Action_Max = atoi(column_value[20]);
	printf("GetRecordCntCfgCallBack  User_Max[%ld]\n Dev_Max[%ld]\n Attr_Max[%ld]\n Agent_Max[%ld]\n TimeCtrl_Max[%ld]\n",
		pStRecordCfg->User_Max, pStRecordCfg->Dev_Max, pStRecordCfg->Attr_Max, pStRecordCfg->Agent_Max, pStRecordCfg->TimeCtrl_Max);
	printf(" Single_Agent_Max[%ld]\n NetSend_Max[%ld]\n NetRecv_Max[%ld]\n NetClientSend_Max[%ld]\n NetClientRecv_Max[%ld]\n",
		pStRecordCfg->Single_Agent_Max, pStRecordCfg->NetSend_Max, pStRecordCfg->NetRecv_Max, pStRecordCfg->NetClientSend_Max, pStRecordCfg->NetClientRecv_Max);
	printf(" ADS_Max[%ld]\n GSM_Max[%ld]\n Action_Max[%ld]\n ",
		pStRecordCfg->ADS_Max, pStRecordCfg->GSM_Max, pStRecordCfg->Action_Max);

	return 0;
}
