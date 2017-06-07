#include <string>
#include <pthread.h>
#include <sys/time.h>
#include "Shared.h"
#include "Init.h"
#include "Define.h"
#include "SqlCtl.h"
#include "Transparent.h"
#include "sqlite3.h"
 
#ifdef  DEBUG
#define DEBUG_TRANSPARENT
#endif

#ifdef DEBUG_TRANSPARENT
#define DBG_TRANSPARENT(a)		printf a;
#else
#define DBG_TRANSPARENT(a)	
#endif


CTransparent::CTransparent()
{
}

CTransparent::~CTransparent()
{
}

bool CTransparent::Initialize(TKU32 unHandleList)
{
    m_unHandleList = unHandleList;
	
	m_ptrHashTable = new HashedTransparentInfo();
	if (!m_ptrHashTable)
	{
		return false;
	}

	//队列处理线程
	char buf[256] = {0};	
	sprintf(buf, "%s %s", "CTransparent", "Thrd");
	if (!ThreadPool::Instance()->CreateThread(buf, &CTransparent::Thrd, true, this))
	{
		DBG(("CTransparent Create failed!\n"))
			return false;
	}
	DBG_TRANSPARENT(("CTransparent::Initialize success!\n "));
	return true; 
}
//队列处理线程
void * CTransparent::Thrd(void *arg)
{
	CTransparent *_this = (CTransparent *)arg;
	_this->ThrdProc();
	return NULL;
}
void CTransparent::ThrdProc(void)
{
    QUEUEELEMENT pMsg = NULL;
	m_ThrdHandle = pthread_self();

	while(1)
	{
		pthread_testcancel();

		//从读卡数据队列中取数据
		if(MMR_OK == g_MsgMgr.GetFirstMsg(m_unHandleList, pMsg, 100))
		{

			QUERY_INFO_CPM_EX* pMsgDeal = (QUERY_INFO_CPM_EX*)pMsg;
			char device_id[DEVICE_ID_LEN_TEMP] = {0};
			char attr_id[DEVICE_ID_LEN_TEMP] = {0};
			memcpy(device_id, pMsgDeal->DevId, DEVICE_ID_LEN);
			memcpy(attr_id, pMsgDeal->DevId + DEVICE_ID_LEN, DEVICE_SN_LEN);

			CTblLock.lock();
			insert_into_transparent_data_table(device_id, attr_id, pMsgDeal->Time, (unsigned char*)pMsgDeal->Value, strlen(pMsgDeal->Value));
			CTblLock.unlock();
			if (NULL != pMsg)
			{
				MM_FREE(pMsg);
				pMsg = NULL;
			}
			continue;
		}		

		//-------------------------------------------------------------
		int cnt = 0;
		time_t tm_NowTime;


		HashedTransparentInfo::iterator it;	
		CTblLock.lock();	
		for (it = m_ptrHashTable->begin(); it != m_ptrHashTable->end(); it++)
		{			
			time(&tm_NowTime);
			cnt++;

			if( tm_NowTime - it->second.ttime >= 30 )//大于30秒未回复,重新发送
			{
				MSG_TRANSPARENT_HASH hash_node;
				memcpy((BYTE*)&hash_node, (BYTE*)&it->second, sizeof(MSG_TRANSPARENT_HASH));
				DBG(("SendNode22[%d][%s][%s][%s][%s]\n", hash_node.Id, hash_node.deviceId, hash_node.attrId, hash_node.ctime, hash_node.data));

				SendToPlatform(hash_node);
				it->second.ttime = tm_NowTime;
			}
		}
		CTblLock.unlock();

		if(0 == cnt)//已经发送完，从数据库取数据进行发送
		{
			query_transparent_data_table();
		}
		usleep( 1000 * 1000);		
	}
}

void CTransparent::SendToPlatform(MSG_TRANSPARENT_HASH hash_node)
{
	if( 1 != g_upFlag)
		return;
	char devicePollId[DEVICE_ATTR_ID_LEN_TEMP] = {0};
	DEVICE_DETAIL_INFO deviceInfo;
	memset((unsigned char*)&deviceInfo, 0, sizeof(DEVICE_DETAIL_INFO));
	memcpy(deviceInfo.Id, hash_node.deviceId, DEVICE_ID_LEN);
	char szUnit[11] = {0};

	memcpy(devicePollId, hash_node.deviceId, DEVICE_ID_LEN);
	memcpy(devicePollId + DEVICE_ID_LEN, hash_node.attrId, DEVICE_SN_LEN);

	DEVICE_INFO_POLL stDevicePoll;
	if (SYS_STATUS_SUCCESS == g_DevInfoContainer.GetDeviceDetailNode(deviceInfo)
		&& SYS_STATUS_SUCCESS == g_DevInfoContainer.GetAttrNode(devicePollId, stDevicePoll))
	{
		if (1 == stDevicePoll.isUpload)
		{
			char updateData[MAXMSGSIZE] = {0};
			char* ptrBuf = updateData;
			memcpy(ptrBuf, hash_node.deviceId, DEVICE_ID_LEN);//deviceId
			ptrBuf += DEVICE_ID_LEN;

			sprintf(ptrBuf, "%-30s", deviceInfo.szCName);//deviceName
			ptrBuf += 30;

			strncpy(ptrBuf, hash_node.attrId, DEVICE_SN_LEN);//attrId
			ptrBuf += DEVICE_SN_LEN;

			sprintf(ptrBuf, "%-20s", stDevicePoll.szAttrName);//deviceName
			ptrBuf += 20;

			sprintf(ptrBuf, "%-20s", hash_node.ctime);
			ptrBuf += 20;

			memcpy(ptrBuf, hash_node.data, hash_node.dataLen);
			ptrBuf += MAX_PACKAGE_LEN_TERMINAL;

			sprintf(ptrBuf, "%-10s", szUnit);
			ptrBuf += 10;

			Up_Msg(hash_node.Id, CMD_TRANSPARENT_DATA_UP, updateData, ptrBuf - updateData);
		}
	}
}
bool CTransparent::Up_Msg(TKU32 Id, int upCmd, char* buf, int bufLen)
{
	bool ret = false;
	if( g_upFlag == 1 )
	{
		char updateData[4096] = {0};
		memset(updateData, 0, sizeof(updateData));
		MsgHdr* pMsgHdr = (MsgHdr*)updateData;
		pMsgHdr->unMsgLen = MSGHDRLEN + 24 + 4 + bufLen;
		pMsgHdr->unMsgCode = 4;

		PMSG_NET_UPLOAD msgUp = (PMSG_NET_UPLOAD)(updateData + MSGHDRLEN);
		
		sprintf(msgUp->szReserve, "%20u", Id);		//保留字
		memset(msgUp->szStatus, 0x30, 4);			//状态
		sprintf(msgUp->szDeal, "%04d", upCmd);
		memcpy(msgUp->szData, buf, bufLen);
		if (!SendMsg_Ex(g_ulHandleNetCliSend, MUCB_IF_NET_CLI_SEND, updateData, pMsgHdr->unMsgLen))
		{
			DBG(("Up_Msg SendMsg_Ex Failed: handle[%d] mucb[%d]\n", g_ulHandleNetCliSend, MUCB_IF_NET_CLI_SEND));
		}
		else
		{
			DBG(("Transparent.cpp 189 [%d] \n", g_ulHandleNetCliSend));
			ret = true;
		}
	}

	return ret;
}

bool CTransparent::Net_Resp_Deal(PMsgRecvFromPl pRcvMsg)
{
	bool ret = false;
	MSG_TRANSPARENT_HASH hash_node;
	char szReserve[21] = {0};
	memcpy(szReserve, pRcvMsg->szReserve, 20);
	hash_node.Id = atoi(szReserve);
	//从hash表删除
	delNode(hash_node);
	//从数据库移除
	char sql[1024] = {0};
	const char* sqlFormat = "delete from transparent_data where sn = %d;";
	memset(sql, 0, sizeof(sql));
	sprintf(sql, sqlFormat, hash_node.Id);
	if(ExecSql(sql, NULL, NULL))
	{
		ret = true;
	}

	return ret;
}
bool CTransparent::insert_into_transparent_data_table(char* device_id, char* attr_id, char* ctime, unsigned char* szdata, int iLen)  //插入Blob记录
{
	bool ret = false;
	char insert[2048] = {0};
	const char* insertFormat = "insert into transparent_data(sn, dev_id, attr_id, CTime, value) values(NULL,'%s',  '%s', '%s', :aaa);";
    sprintf(insert, insertFormat, device_id, attr_id, ctime);
	
	unsigned char data[1024] = {0};
	int  dataLen = 0;
	if(1 > (dataLen = String2Hex(data, szdata, iLen)))
		return false;

	sqlite3* db = 0;
	
	if (SQLITE_OK == sqlite3_open("/home/CPM/transparent.db", &db))
	{
		sqlite3_stmt* stat = 0;
		sqlite3_busy_timeout(db, 200);	

		DBG(("insert[%s]\n", insert));
		int dbResult = SYS_STATUS_FAILED;
		do
		{
			int dbret = 0;
			if(SQLITE_OK != (dbret = sqlite3_prepare_v2(db, insert, strlen(insert), &stat, 0)))
			{		
				DBG(("insert_into_transparent_data_table sqlite3_prepare failed dbret[%d] \n",dbret));
				break;
			}
			int index = sqlite3_bind_parameter_index(stat, ":aaa");
			if(SQLITE_OK != sqlite3_bind_blob(stat, index, data, dataLen, NULL))
			{			
				sqlite3_finalize(stat);
				DBG(("insert_into_transparent_data_table sqlite3_bind_blob failed dbret[%d]\n",dbret));
				break;
			}

			if(SQLITE_DONE != sqlite3_step(stat))   
			{				
				sqlite3_finalize(stat);
				DBG(("insert_into_transparent_data_table sqlite3_step failed dbret[%d]\n",dbret));
				break;
			}
			dbResult = SYS_STATUS_SUCCESS;
			ret = true;
			sqlite3_finalize(stat);
			
		}while(SYS_STATUS_DB_LOCKED_ERROR == dbResult);	 
		int closeret;
		if(SQLITE_OK != (closeret = sqlite3_close(db)))
			printf("DB sqlite3_close failed ret[%d]\n", closeret);
	}
	else
	{
		printf("DB open failed\n");
	}

	DBG(("insert_into_transparent_data_table end\n"));
	return ret;
}
bool CTransparent::query_transparent_data_table()  //插入Blob记录
{
	bool ret = false;
	int rc;
	const char* sql = "select * from transparent_data order by ctime asc limit 0,128;";

	sqlite3* db = 0;

	if (SQLITE_OK == sqlite3_open("/home/CPM/transparent.db", &db))
	{
		sqlite3_stmt* stat = 0;
		sqlite3_busy_timeout(db, 200);	
		int dbResult = SYS_STATUS_FAILED;
		do
		{
			if(SQLITE_OK != (rc = sqlite3_prepare(db, sql, strlen(sql), &stat, 0)))
			{
				//DBG(("query_transparent_data_table sqlite3_prepare failed dbret[%d] \n",rc));
				break;
			}
			rc = sqlite3_step(stat);
			int ncols = sqlite3_column_count(stat);
			while(rc == SQLITE_ROW) 
			{
				MSG_TRANSPARENT_HASH transparentNode;
				memset((unsigned char*)&transparentNode, 0, sizeof(MSG_TRANSPARENT_HASH));
				transparentNode.ttime = 0;
				for(int i=0; i < ncols; i++) 
				{
					switch(i)
					{
						case 0://sn, int
							transparentNode.Id = sqlite3_column_int(stat, i);
							break;
						case 1://deviceid,str
							memcpy(transparentNode.deviceId, sqlite3_column_text(stat, i), DEVICE_ID_LEN);
							break;
						case 2://attrid, str
							memcpy(transparentNode.attrId, sqlite3_column_text(stat, i), DEVICE_SN_LEN);
							break;
						case 3://time, str
							strcpy(transparentNode.ctime,  (char*)sqlite3_column_text(stat, i));
							break;
						case 4://value blob
							{
								//得到字段中数据的长度 
								transparentNode.dataLen = sqlite3_column_bytes(stat, i); 
								memcpy(transparentNode.data, sqlite3_column_blob(stat, i), transparentNode.dataLen);
							}
							break;
					}
				}
				if(addNode(transparentNode))
				{
					DBG(("tranNode[%d][%s][%s][%s][%s]\n", transparentNode.Id, transparentNode.deviceId, transparentNode.attrId, transparentNode.ctime, transparentNode.data));
				}
				
				rc = sqlite3_step(stat);
			}

			sqlite3_finalize(stat);

			dbResult = SYS_STATUS_SUCCESS;
			ret = true;
			
		}while(SYS_STATUS_DB_LOCKED_ERROR == dbResult);	 

		int closeret;
		if(SQLITE_OK != (closeret = sqlite3_close(db)))
			printf("DB sqlite3_close failed ret[%d]\n", closeret);
	}
	else
	{
		printf("DB open failed\n");
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
bool CTransparent::ExecSql(char *sql, sqlite3_callback FuncSelectCallBack, void * userData)
{
	bool ret = false;
	char* errmsg = NULL;
	sqlite3* db = NULL;
	
	if (SQLITE_OK == sqlite3_open("/home/CPM/transparent.db", &db))
	{
		sqlite3_busy_timeout(db, 200);
		DBG(("ExecSql--------sql[%s]\n", sql));
		
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
				if (NULL != strstr(errmsg, "database is locked"))//数据库被锁
				{
					dbResult = SYS_STATUS_DB_LOCKED_ERROR;
				}
				DBG(("DB ERROR:%s, %s\n",errmsg, sql));
			}
			sqlite3_free(errmsg);
		}while(SYS_STATUS_DB_LOCKED_ERROR == dbResult);
		
		sqlite3_close(db);
	}
	
	db = NULL;
	return ret;
}
bool CTransparent::addNode(MSG_TRANSPARENT_HASH hash_node)
{
    int ret = false;
    MSG_TRANSPARENT_HASH msg_node;
    HashedTransparentInfo::iterator it;

    memset((BYTE*)&msg_node, 0, sizeof(msg_node));
    memcpy((BYTE*)&msg_node, (BYTE*)&hash_node, sizeof(MSG_TRANSPARENT_HASH));

	CTblLock.lock();
    it = m_ptrHashTable->find((TKU32)msg_node.Id);
	if (it == m_ptrHashTable->end())
	{		
		m_ptrHashTable->insert(HashedTransparentInfo::value_type((TKU32)msg_node.Id,msg_node));
		ret = true;
	}
	CTblLock.unlock();
    return ret;
}
bool CTransparent::delNode(MSG_TRANSPARENT_HASH hash_node)
{
    HashedTransparentInfo::iterator it;
	CTblLock.lock();
    it = m_ptrHashTable->find(hash_node.Id);	
	if (it != m_ptrHashTable->end())
	{	
		m_ptrHashTable->erase(it);
	}
	CTblLock.unlock();
	return true;
}
/*******************************END OF FILE************************************/


