#ifndef _ON_TIME_CTRL_
#define _ON_TIME_CTRL_

#include <tr1/unordered_map>
#include <pthread.h>
#include <time.h>
#include <string>
#include "Shared.h"
using namespace std;

#pragma pack(1)

typedef struct _ONTIME_ACT_HASH
{
    TKU32   Seq;                //唯一键
    char    id[DEVICE_ACT_ID_LEN_TEMP];             //devid+sn
    time_t  actTime;            //下次触发时间
    char    actValue[256];      //动作内容
	char	szTime[13];
}ONTIME_ACT_HASH, *PONTIME_ACT_HASH;
#pragma pack()

class COnTimeCtrl
{

public:
	COnTimeCtrl();
	virtual ~COnTimeCtrl();

	bool Initialize();

public:
	void OnTimeCtrlProc(void);

public:
    time_t getNextActTime(char* shortTime);

    bool addOnTimeNode(ONTIME_ACT_HASH hash_node);
    bool GetAndDeleteOnTimeNode(ONTIME_ACT_HASH& hash_node);
    bool GetOnTimeHashData(ONTIME_ACT_HASH& hash_node);

private:

    bool initOntimeInfo();
    static int initOnTimeInfoCallBack(void *data, int n_column, char **column_value, char **column_name);
    int initOnTimeInfoCallBackProc(int n_column, char **column_value, char **column_name);
   
	//hash表
private:
    //定时任务哈希表相关
	typedef  tr1::unordered_map<TKU32, ONTIME_ACT_HASH> HashedOnTimeActInfo;
	HashedOnTimeActInfo*  m_ptrOnTimeActInfoTable;
   

	class Lock
	{
	private:
		pthread_mutex_t	 m_lock;
	public:
		Lock() { pthread_mutex_init(&m_lock, NULL); }
		bool lock() { return pthread_mutex_lock(&m_lock)==0; }
		bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
	}CTblLock;

private:
	TKU32 m_SeqCPM;
};

#endif
