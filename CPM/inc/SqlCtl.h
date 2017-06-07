#ifndef _SQLCTL_H
#define _SQLCTL_H
#include <pthread.h>
#include "sqlite3.h"
#include "Define.h"

typedef enum _DB_ERROR_LVL
{
	DB_ERROR_NO,
	DB_ERROR_SERIOUS,
	DB_ERROR_NORMAL
}DB_ERROR_LVL;

typedef struct _DB_COPY_PARA
{
	char db[128];
	char table[56];
}DB_COPY_PARA, *PDB_COPY_PARA;

typedef struct _LEDGER_DEVICE
{
	char id[DEVICE_ID_LEN_TEMP];
	char attrId[DEVICE_SN_LEN_TEMP];
}LEDGER_DEVICE, *PLEDGER_DEVICE;

typedef struct _LEDGER_DEVICE_INFO
{
	int iDevCnt;
	LEDGER_DEVICE stLedgerDevice[512];
}LEDGER_DEVICE_INFO, *PLEDGER_DEVICE_INFO;

typedef struct _LEDGER_USER_INFO
{
	int iCnt;
	char szUserId[512][5];
}LEDGER_USER_INFO, *PLEDGER_USER_INFO;

typedef struct _DB_CONFIG
{
	int iDataMaxCnt;
	int iHourLedgerMaxCnt;
	int iDayLedgerMaxCnt;
	int iAlertMaxCnt;
	int iAlarmMaxCnt;
	int iAtsInfoMaxRecode;
	int iAtsLdgMaxRecode;
	int iDataStoreMode;
}DB_CONFIG, *PDB_CONFIG;


class CSqlCtl
{
public:
    CSqlCtl(void);
    ~CSqlCtl(void);

	bool Initialize(const char* dbname);
	void getDBConfig(DB_CONFIG& dbConfig);

	void getModeConfig(MODE_CFG& modeConfig);
	static int GetModeCfgCallBack(void *data, int n_column, char **column_value, char **column_name);    //查询时的回调函数;

	void getRecordCountConfig(RECORD_COUNT_CFG& recordConfig);
	static int GetRecordCntCfgCallBack(void *data, int n_column, char **column_value, char **column_name);    //查询时的回调函数;

    bool error(const char* error);                      //打印错误信息
    bool insert_into_table(const char *table, char *record, bool bRedo);  //插入记录
    bool update_into_table(const char *table, char *record, bool bRedo); 

    int select_from_table(const char* dbName, char *table, char *column, char *condition, char *result);
	int select_from_table_no_lock(const char* dbName, char *table, char *column, char *condition, char *result);
	int select_from_tableEx(char *sql, sqlite3_callback FuncSelectCallBack, void *result);

    bool delete_record(const char *table,char *expression, bool bRedo);   //删除记录
	bool close_database();
    static int loadData(void *data, int n_column, char **column_value, char **column_name);

	bool ExecSql(const char *sql, sqlite3_callback FuncSelectCallBack, void * userData, bool bRedo, time_t tTime);
	bool CPM_ExecSql(const char *sql, sqlite3_callback FuncSelectCallBack, void * userData, bool bRedo, time_t tTime);
public:
	bool InitializeTempDB();
	bool InitializeCPMDB();
	bool UpdateHistoryDB();
private:
	bool UpdateDB(const char* src, const char* dst, DB_CONFIG dbConfig);
	bool UpdateTable(const char* src, const char* dst, const char* tablename);
	bool IncrementTable(const char* src, const char* dst, const char* tablename, const char* tarTableName, int record_Max);
	bool IncrementData(const char* src, const char* dst, DB_CONFIG dbConfig);
	bool RecoverDB(const char* src, const char* dst, const char* sample);
	bool RecoverDB_New(const char* dst);

	bool CopyTable(const char* src, const char* dst, const char* tablename);
	//static int CallBackCopyTable(void *data, int n_column, char **column_value, char **column_name);    //复制表数据的回调

private:
	bool Ledger();
	static int GetLedgerDeviceCallBack(void *data, int n_column, char **column_value, char **column_name);
	static int GetLedgerUserCallBack(void *data, int n_column, char **column_value, char **column_name);    //查询时的回调函数;

	bool doAvgLedger(struct tm tmNow);
	bool doCountLedger(struct tm tmNow);
	bool doTimeCheckLedger(struct tm tmNow);
public:
	bool AppAddDeviceDetail(char* id, char* cname, char* upper, char* upper_channel, char* ctype, char* self_id, char status, char* private_attr, char* brief, char* reserve2, char* memo);
	static int SelectFromDeviceAttrCallBack(void *data, int n_column, char **column_value, char **column_name);    //查询时的回调函数
	static int GetDefaultPrivateAttr(char* attrDefine, char* private_attr);    //采集属性私有参数初始话
	static int SelectFromDeviceCmdCallBack(void *data, int n_column, char **column_value, char **column_name);    //查询时的回调函数
	

	static void InsertAlertInfoTable(char* deviceId, char* attrId, char* time, int alarmLevel , char* alarmDescribe, char status, char* level);
	static void DeleteFromAlertNowTable(char* deviceId, char* attrId, int nRecordeCount);
	static void UpdateAlertInfoTable(char* deviceId, char* attrId, char status, char* time);

	int GetUserInfoByCardId(char* szCardId, char* szUserId, char* szUserName);
private:
	class Lock
	{
	private:
		pthread_mutex_t	 m_lock;
	public:
		Lock() { pthread_mutex_init(&m_lock, NULL); }
		bool lock() { return pthread_mutex_lock(&m_lock)==0; }
		bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
	}CTblLockDb;

private:
   // sqlite3* db;
    char **strTable;

	char m_dbPath[128];
};

extern CSqlCtl g_SqlCtl;
#endif
