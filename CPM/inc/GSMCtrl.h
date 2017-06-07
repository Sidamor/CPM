#ifndef _GSM_CTRL_
#define _GSM_CTRL_

#include <tr1/unordered_map>
#include <pthread.h>
#include <time.h>
#include <string>
#include "Shared.h"
using namespace std;
#pragma pack(1)
typedef struct _GSM_ACTION_HASH
{
	TKU32   Seq;				 //编号
	char	Tel[20];			 //源手机号
	char	Value[256];			 //源内容
	time_t	act_time;
	int		source;				//来源,0,联动指令；1,嵌入式Web指令；2,平台指令
}GSM_ACTION_HASH, *PGSM_ACTION_HASH;


#pragma pack()

class CGSMCtrl
{

public:
	CGSMCtrl();
	virtual ~CGSMCtrl();

	bool Initialize(TKU32 unHandleSend);

public:
	static void* SMSCtrlThrd(void *arg);
	void SMSCtrlThrdProc(void);

public:
	static int SendToSmsList(char* Tel, char* content);
	int SendProInfoToSmsMonthly();
private:
	TKU32 GetSeq();
	//int GetSmsInfoByActionMsg(ACTION_MSG stAction, SMS_INFO& stSms);
private:
	int GsmRequestProc(SMS_INFO* pSms);
	int CheckSmsPwd(char* pwd);
	int SetPro_Date(char* proDate);
	int SetPro_Tel(char* proTel);

	int GsmCtrlProc( char* Tel, char* deviceId, char* actionId, char* actionValue);
	int SendProInfoToSms(char*  Tel);
	int SendDeviceValuesToSms(char*  Tel, char* deviceId);
	static int SelectCallBack_DevDataInfo(void *data, int n_column, char **column_value, char **column_name);
	int GetRoleArea(const char*  Tel, char* hj_role);//lhy 查询所有
	int CheckTel(const char*  Tel);
	int CheckDeviceAllow(const char*  Tel, char* deviceId);
	int GetDevList(char*  roleid, char* deviceList);

	int CheckActionId(char* tel, char* deviceId, char* actionSn);
	int ActionResponseProc( PACTION_RESP_MSG pActionRespMsg);


	//hash表
private:

	typedef  tr1::unordered_map<TKU32, GSM_ACTION_HASH> HashedActInfo;
	HashedActInfo*  m_ptrActInfoTable;

	bool addNode(GSM_ACTION_HASH hash_node);
	bool GetAndDeleteHashData(GSM_ACTION_HASH& hash_node);
	bool GetHashData(GSM_ACTION_HASH& hash_node);

   	class LockGsmAction
	{
	private:
		pthread_mutex_t	 m_lock;
	public:
		LockGsmAction() { pthread_mutex_init(&m_lock, NULL); }
		bool lock() { return pthread_mutex_lock(&m_lock)==0; }
		bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
	}CTblLockGsmAction;

private:
	pthread_t m_SMSSendThrd;
	TKU32 m_unHandleGSM;              //GSM处理队列句柄 

	TKU32 m_SeqCPM;
};

#endif
