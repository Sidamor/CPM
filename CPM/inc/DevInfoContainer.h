#ifndef _DEV_INFO_CONTAINER_H_
#define _DEV_INFO_CONTAINER_H_

#include <stdio.h>
#include <stdlib.h>

#include <tr1/unordered_map>
#include <pthread.h>
#include "Define.h"
using namespace std;

typedef struct _DEVICE_DETAIL_INFO
{
	char Id[DEVICE_ID_LEN_TEMP];			//设备编号
	char Upper[11];			//上级设备编号lsd
	char szCName[31];		//设备名
	char cStatus;			//设备状态
	char szSelfId[21];		//子通道号
	char szBreaf[21];		//简称
	char szCurrentMode[DEVICE_ACTION_SN_LEN_TEMP];   //当前模式值
	char szModList[513];	//模式列表
	char szPwd[64];			//设备密码lsd
	int  iChannel;
}DEVICE_DETAIL_INFO, *PDEVICE_DETAIL_INFO;

typedef struct _DEVICE_MODE_INFO
{
	char Id[DEVICE_MODEID_LEN_TEMP];			//设备编号
	char curActionId[DEVICE_ACTION_SN_LEN_TEMP];		//模式编号
	char szModeName[56];	//模式名称
	char szActionName[21];	//动作名称
}DEVICE_MODE_INFO, *PDEVICE_MODE_INFO;

class CDevInfoContainer
{
public:
	CDevInfoContainer();
	virtual ~CDevInfoContainer(void);

	virtual bool Initialize();	
	void Terminate();
public:
	int InsertQueryNode(DEVICE_INFO_POLL& new_node);  //将结点插入到hash表公共接口
	int DeleteQueryNode(DEVICE_INFO_POLL new_node);	  //删除结点

	
private:
	int doInsertQueryNode(DEVICE_INFO_POLL& new_node);   //将结点插入到hash表私有接口
	int doDeleteQueryNode(DEVICE_INFO_POLL new_node);	//删除结点

private:
	bool InitDataType();
	bool InitDataTypeProc(const char* sql);
	static int InitDataTypeCallBack(void *data, int n_column, char **column_value, char **column_name);
	int InitDataTypeCallBackProc(int n_column, char **column_value, char **column_name);
public:
	int GetParaUnit(int paraType, char* outBuf);

private:	
	bool InitDeviceInfo();

	int SendToGsm(char* srcDev, char* srcSn,  char *deviceId, char* dstActionSn, char *smsValue, int type, TKU32 Seq);

private:
	class LockDevice
	{
	private:
		pthread_mutex_t	 m_lock;
	public:
		LockDevice() { pthread_mutex_init(&m_lock, NULL); }
		bool lock() { return pthread_mutex_lock(&m_lock)==0; }
		bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
	}CTblLockDevice;
	typedef  tr1::unordered_map<string, DEVICE_DETAIL_INFO> HashedDeviceDetail;
	HashedDeviceDetail* m_HashTableDevice;      //采集属性哈希表
	bool InitDeviceDetailInfo(char* sql);
	static int InitDeviceDetailCallBack(void *data, int n_column, char **column_value, char **column_name);    //回调函数
	int InitDeviceDetailCallBackProc(int n_column, char **column_value, char **column_name);

public:
	class LockMode
	{
	private:
		pthread_mutex_t	 m_lock;
	public:
		LockMode() { pthread_mutex_init(&m_lock, NULL); }
		bool lock() { return pthread_mutex_lock(&m_lock)==0; }
		bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
	}CTblLockMode;
	typedef  tr1::unordered_map<string, DEVICE_MODE_INFO> HashedDeviceMode;
	HashedDeviceMode* m_HashTableMode;

	int InitDeviceModeNode(char* deviceId, char* modeList);
	int GetDeviceModeNode(DEVICE_MODE_INFO& new_node);
	int InitDeviceModeNodeCurValue(char* deviceId, char* actionId, char* actionName, char flag, char flag2);
	int UpdateDeviceDetailMode(char* deviceId, char* actionId);
	int UpdateDeviceModeNodeCurValue(char* deviceId, char* actionId, char* actionName);
	int InsertDeviceModeNode(DEVICE_MODE_INFO new_node);
	int UpdateDeviceModeNode(DEVICE_MODE_INFO new_node);
	int DeleteDeviceModeNode(DEVICE_MODE_INFO new_node);
	int ModeHashDeleteByDevice(char* deviceId);

public:
	int GetDeviceStatusInfo(char* deviceId, DEVICE_DETAIL_STATUS& stPollNode);
	int GetDeviceParasInfo(char* deviceId, DEVICE_DETAIL_PARAS& stPollNode, void* msg, int& seq);
	int GetDeviceModesInfo(char* deviceId, DEVICE_DETAIL_MODES& stPollNode, void* msg, int& seq);
	int GetDeviceDetailNode(DEVICE_DETAIL_INFO& new_node);
	int InsertDeviceDetailNode(DEVICE_DETAIL_INFO& new_node);
	int UpdateDeviceDetailNode(DEVICE_DETAIL_INFO& new_node);
	int DeleteDeviceDetailNode(DEVICE_DETAIL_INFO new_node);	
	int UpdateDeviceDetailNodeCurMode(DEVICE_DETAIL_INFO new_node);
	bool HashDeviceStatusUpdate(char* device_id, int status);
	
private://hashtable 相关
	class LockRoll
	{
	private:
		pthread_mutex_t	 m_lock;
	public:
		LockRoll() { pthread_mutex_init(&m_lock, NULL); }
		bool lock() { return pthread_mutex_lock(&m_lock)==0; }
		bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
	}CTblLockRoll;

	class LockAction
	{
	private:
		pthread_mutex_t	 m_lock;
	public:
		LockAction() { pthread_mutex_init(&m_lock, NULL); }
		bool lock() { return pthread_mutex_lock(&m_lock)==0; }
		bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
	}CLockAction;

	typedef  tr1::unordered_map<string, DEVICE_INFO_POLL> HashedDeviceRoll;
	HashedDeviceRoll* m_HashTableRoll;      //采集属性哈希表
	static int InitDeviceRollCallBack(void *data, int n_column, char **column_value, char **column_name);    //回调函数
	int InitDeviceRollCallBackProc(int n_column, char **column_value, char **column_name);

	typedef  tr1::unordered_map<string, DEVICE_INFO_ACT> HashedDeviceAction;
	HashedDeviceAction* m_HashTableAction;      //动作属性哈希表
	static int InitDeviceActionCallBack(void *data, int n_column, char **column_value, char **column_name);    //回调函数
	int InitDeviceActionCallBackProc(int n_column, char **column_value, char **column_name);

public:

	int InitDeviceIndex(DeviceInit_CallBack FuncSelectCallBack, void * userData);
	int GetDeviceInfoNode(char* deviceId, DEVICE_DETAIL& stPollNode);
	int GetDeviceRealInfo(char* deviceId, DEVICE_DETAIL& stPollNode);

	int GetShowString(char* szValue, char* szDefine, char* outBuf);

	int GetAttrNode(char* deviceAttrId, DEVICE_INFO_POLL& stPollNode);
	int GetNextAttrNode(char* startAttr, char* deviceAttrId, DEVICE_INFO_POLL& stPollNode);

	int GetAttrValue(char* deviceAttrId, LAST_VALUE_INFO& stLastValue);
	bool IsDeviceOnline(char* deviceId);

	int GetActionNode(char* deviceActionId, DEVICE_INFO_ACT& stPollNode);

	//V1.0.0.17接口
	int GetNextPollAttrNode(const int iChannelId, const char* pDevAttrId, DEVICE_INFO_POLL& stNewNode);
	int GetFirstPollAttrNode(int iChannleId, DEVICE_INFO_POLL& stNewNode);
	
	int GetDeviceRollByDevId(char* devId, DEVICE_INFO_POLL& stPollNode);
	int GetDeviceRollByChannelId(int channelId, DEVICE_INFO_POLL& stPollNode);
	int GetDeviceRollByChannelDevTypeId(int iChannel, char* deviceType, char* attrId, DEVICE_INFO_POLL& stPollNode);

	int GetDeviceRollBySpecialDevTypeId(char* deviceType, char* attrId, DEVICE_INFO_POLL& stPollNode);

	static int DisPatchAction(ACTION_MSG stActMsg);
	static void DisPatchQueryAction(char* queryAttrId);
	static void SendToActionSource(ACTION_MSG actionMsg, int actionRet);
	static void InsterIntoAlarmInfo(ACTION_MSG actionMsg, int actRet);

	bool InitDeviceRollInfo(char* sql);
	int AddDeviceRoll(DEVICE_INFO_POLL node);
	int UpdateDeviceRoll(DEVICE_INFO_POLL node);
	int UpdateDeviceRollLastValue(DEVICE_INFO_POLL node, char* lastValue);
	int UpdateDeviceRollRetryCount(DEVICE_INFO_POLL node);
	int UpdateDeviceAlarmValue(DEVICE_INFO_POLL node);
	int DeleteDeviceRoll(DEVICE_INFO_POLL node);

	static int ADCValueNoticeCallBack(void* userData, int iChannel, unsigned char* value, int len);
	int UpdateDeviceRollNodeByChannel(int iChannel, unsigned char* value, int len, bool isSend);//更新模拟量、开关量口的数据
	int UpdateDeviceRollNodeByChannelDevType(int iChannel, char* devType, char* attrId,  unsigned char* value, int len, bool isSend);


	bool InitDeviceActionInfo(char* sql);
	int GetDeviceActionNode(DEVICE_INFO_ACT& node);
	int AddDeviceAction(DEVICE_INFO_ACT node);
	int UpdateDeviceAction(DEVICE_INFO_ACT node);
	int DeleteDeviceAction(DEVICE_INFO_ACT node);
	int UpdateDeviceActionStatus(char* deviceId, char* deviceSn, char cBeOn, char* actName, char* vDevId, char* vActionId);

public:
	int QueryHashUpdateByDevice(NET_DEVICE_INFO_UPDATE new_node);
	int QueryHashDeleteByDevice(NET_DEVICE_INFO_UPDATE new_node);
	int QueryHashUpdateByDeviceAttr(NET_DEVICE_ATTR_INFO_UPDATE new_node);
	int QueryHashDeleteByDeviceAttr(char* device_attr);
	bool HashDeviceDefenceSet(char* dev_attr_id, int status);
	bool PrintPollHash();

	bool delNode(DEVICE_INFO_POLL hash_node);
	bool GetHashData(DEVICE_INFO_POLL& hash_node);

	bool ActionHashUpdateByNet(NET_DEVICE_INFO_UPDATE new_node);
	int ActionHashDeleteByNet(NET_DEVICE_INFO_UPDATE new_node);
	bool delNodeAct(DEVICE_INFO_ACT hash_node);

	int SetDeviceLineStatus(char* deviceChannelId, bool bStatus);


	//单位表
	PARA_INFO m_ParaInfo[9999];
};

#endif
