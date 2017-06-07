#ifndef _ANALYZER_H
#define _ANALYZER_H


#include "Shared.h"
#include "Define.h"
#include <sys/time.h>

#include <tr1/unordered_map>
using namespace std;

#define MAX_CNT_ACTFORMAT  (2048)//V1.0.1.2
#define MAX_CNT_ATTRACT	   (2048)//V1.0.1.2
#define MAX_CNT_AGENT	   (2048)//V1.0.1.2

typedef struct _ACT_FORMAT    //单条联动配置存储
{
	int iSeq;				//流水号
    char szCondition[129];       //联动配置生效条件
    char szValibtime[31];        //联动配置生效时间限制
    int  iInterval;       //联动间隔
	int  iTimes		;       //联动次数
    char szActDevId[DEVICE_ID_LEN_TEMP];              //目标设备ID
    char szActActionId[DEVICE_ACTION_SN_LEN_TEMP];               //目标设备sn
    char szActValue[256];         //联动内容
    time_t time_lastTime;     //上次联动时间
	int	times_lastTime;			//上次联动次数
}ACT_FORMAT, *PACT_FORMAT;

typedef union _LastValue{
	short			shortValue;
	int				iValue;
	float			fValue;
	double		    dfValue;
	unsigned char	szbValue[128];
	char			cValue;
	char			szcValue[128];
	unsigned char	szBcdValue[128];
}UnionLastValue;

typedef struct _MSG_ANALYSE_HASH
{
    char key[DEVICE_ATTR_ID_LEN_TEMP];              //设备ID号
	char dataStatus;           //设备自定义状态
	char deviceStatus;         //设备自定义状态
    char standard[60];         //标准范围
    char statusDef[128];       //设备状态转换表
    int  actCnt;
	UnionLastValue unionLastValue;
	struct timeval LastTime;            //上次采样时间
	int agentSeq[MAX_CNT_ACTFORMAT];
}MSG_ANALYSE_HASH,*PMSG_ANALYSE_HASH;
class CAnalyser
{
public:
    CAnalyser(void);
	virtual ~CAnalyser(void);

    virtual bool Initialize(TKU32 unHandleAnalyser);
    bool addNode(MSG_ANALYSE_HASH hash_node);              //哈希表增加节点
    bool delNode(MSG_ANALYSE_HASH hash_node);              //哈希表删除节点
	bool updateNodePara(MSG_ANALYSE_HASH hash_node, int paraId);
    bool searchNode(MSG_ANALYSE_HASH hash_node);           //在哈希表里查找节点

    bool insertActData(char *inMsg);
    bool GetHashData(MSG_ANALYSE_HASH& hash_node);          //取得哈希表节点内容
    static int InitActHashCallback(void *data, int n_column, char **column_value, char **column_name);
    int InitActHashCallbackProc(int n_column, char **column_value, char **column_name);
    void f_DelData();

	bool InitAgentInfo(const char* sqlData);
	static int InitAgentInfoCallback(void *data, int n_column, char **column_value, char **column_name);    //查询联动表时的回调函数
	int InitAgentInfoCallbackProc(int n_column, char **column_value, char **column_name);

private:
    static void *pAnalyserThrd(void* arg);
    void MsgAnalyzer();              //解析线程函数

	void UpdateDeviceStatus(char* deviceId, char* status, char* strTime);//设备状态变更
	void InsertDataTable(MSG_ANALYSE_HASH hash_node, const char* tableName, char* time, char* szValue , int status, bool bRedo);
	void InsertATSTable(MSG_ANALYSE_HASH hash_node, char* szTableName, char* time, char* szValue , char* pUserId);

    void DecodeAct(char* deviceId, const char* sn, int vType, char* value, int* pAgentSeq, int actCnt, char* strTime);    //数据解析

	void DoOffLineAct(char* deviceId, const char* sn, int* pAgentSeq,  int actCnt, bool bIsReNotice);
    
    bool InitAnalyseHash();          //哈希表数据初始化
    void getAgentMsg(int* pAgentSeq,   AgentInfo stAgent);   //生成联动配置存储表
    bool conditionCheck(int vType, const char* value, char* split_result_pos);    //条件匹配
	bool f_valueChangedCheck(int vType, char* value, UnionLastValue unionLastValue);
    bool f_timeCheck(time_t tm_NowTime, char *timeFormat);
    bool deviceStatusUpdate(char* devId, int status);
    int analogCheck( int channel, int baseValue, float analogValue );    //测试模拟量准确度验证
    int switchCheck( int channel, float baseValue, int switchValue );    //开关量准确度验证

    class Lock
	{
	private:
		pthread_mutex_t	 m_lock;
	public:
		Lock() { pthread_mutex_init(&m_lock, NULL); }
		bool lock() { return pthread_mutex_lock(&m_lock)==0; }
		bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
	}CTblLock;

private:                         //哈希表节点数
	typedef  tr1::unordered_map<string, MSG_ANALYSE_HASH> HashedInfo;
    HashedInfo* m_pActHashTable;           //设备动作哈希表
    TKU32 m_unHandleAnalyser;              //处理队列句柄

	typedef  tr1::unordered_map<int, ACT_FORMAT> HashedAgentInfo;
	HashedAgentInfo* m_pAgentHashTable;

public:
	bool InitActHashData(char* sqlData);
	bool AnalyseHashDeleteByNet( char* device_id);
	bool AnalyseHashDeleteByDeviceAttr( char* device_attr_id);

	int AnalyseHashAgentAdd( AgentInfo stAgentInfo);
	int AnalyseHashAgentUpdate(AgentInfo stAgentInfo);
	bool AnalyseHashAgentDelete(AgentInfo stAgentInfo);
	int AnalyseHashAgentCheck( AgentInfo stAgentInfo);

public:
	bool ConditionCheck(char* attrId, int vType, const char* curValue, char* strConditions);
	bool IsTrue(char* pAttrId, int vType, const char* curValue, char* expression);

	void transPolish1(char* szRdCalFunc, char* tempCalFunc);
	bool IsAlp(char c);
	bool IsDigit(char* ch);
	bool IsAttrId(char* ch);
	int compvalue1(char* pAttrId, int vType, const char* curValue, char* tempCalFunc, float& fRsult);

};


#endif

