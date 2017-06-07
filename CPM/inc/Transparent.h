#ifndef _TRANSPARENT_H
#define _TRANSPARENT_H

#include <tr1/unordered_map>
#include "Shared.h"
#include "Define.h"
using namespace std;

typedef struct _MSG_TRANSPARENT_HASH
{
	TKU32   Id;				 //编号
	time_t	ttime;    
	char ctime[21];          //数据采集时间
	char deviceId[DEVICE_ID_LEN_TEMP];
	char attrId[DEVICE_SN_LEN_TEMP];
	int dataLen;
	unsigned char data[MAX_PACKAGE_LEN_TERMINAL];
}MSG_TRANSPARENT_HASH, *PMSG_TRANSPARENT_HASH;

class CTransparent
{
public:
    CTransparent(void);
	virtual ~CTransparent(void);
	virtual bool Initialize(TKU32 unHandleList);

	bool Net_Resp_Deal(PMsgRecvFromPl pRcvMsg);
private:
    TKU32 m_unHandleList;              //处理队列句柄
	pthread_t m_ThrdHandle;
public:
	static void * Thrd(void *arg);
	void ThrdProc(void);
private:
	typedef  tr1::unordered_map<TKU32, MSG_TRANSPARENT_HASH> HashedTransparentInfo;
	HashedTransparentInfo*  m_ptrHashTable;

	class Lock
	{
	private:
		pthread_mutex_t	 m_lock;
	public:
		Lock() { pthread_mutex_init(&m_lock, NULL); }
		bool lock() { return pthread_mutex_lock(&m_lock)==0; }
		bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
	}CTblLock;

	bool addNode(MSG_TRANSPARENT_HASH hash_node);
	bool delNode(MSG_TRANSPARENT_HASH hash_node);

private:
	bool Up_Msg(TKU32 Id, int upCmd, char* buf, int bufLen);
	void SendToPlatform(MSG_TRANSPARENT_HASH hash_node);
	
    bool insert_into_transparent_data_table(char* device_id, char* attr_id, char* CTime, unsigned char* data, int dataLen);  //插入Blob记录
	bool query_transparent_data_table();
	bool ExecSql(char *sql, sqlite3_callback FuncSelectCallBack, void * userData);

};


#endif

