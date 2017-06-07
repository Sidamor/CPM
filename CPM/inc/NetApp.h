#ifndef __NET_APP_H__
#define __NET_APP_H__

#include "Define.h"
#pragma pack(1)

//2020 设备状态查询
typedef struct _StrNetDevStatusQuery{      
	char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
	char szIds[1024];
}StrNetDevStatusQuery, *PStrNetDevStatusQuery;

//2021 设备属性数据查询
typedef struct _StrNetDevParasQuery{      
	char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
	char szIds[1024];
}StrNetDevParasQuery, *PStrNetDevParasQuery;

//2022 设备模式数据查询
typedef struct _StrNetDevModesQuery{      
	char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
	char szIds[1024];
}StrNetDevModesQuery, *PStrNetDevModesQuery;

//2000 设备状态查询
typedef struct _StrNetDevQuery{      
	char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
	char szDevType[DEVICE_TYPE_LEN];				//设备型号
	char szId[DEVICE_INDEX_LEN];					//设备编号
}StrNetDevQuery, *PStrNetDevQuery;

//2001/2002 设备添加/修改
typedef struct _StrNetDevUpdate{      
	char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
	char szDevType[DEVICE_TYPE_LEN];				//设备型号
	char szId[DEVICE_INDEX_LEN];					//设备编号
	char szReserve1[DEVICE_ID_LEN];				//上级设备
	char szDevAppType[2];			//设备应用类型
	char szChannelId[2];			//通道编号
	char szSelfId[20];				//子通道号
	char szCName[30];				//设备名称
	char szBrief[20];				//设备简称
	char szPrivateAttr[255];		//设备私有参数
	char szReserve2[128];		//设备私有参数
	char szMemo[128];			//设备备注
}StrNetDevUpdate, *PStrNetDevUpdate;

//2003 设备删除
typedef struct _StrNetDevDel{      
	char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
	char szDevType[DEVICE_TYPE_LEN];				//设备型号
	char szId[DEVICE_INDEX_LEN];					//设备编号
}StrNetDevDel, *PStrNetDevDel;

//2005 设备采集属性添加/修改
typedef struct _StrNetDevAttrUpdate{      
	char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
	char szDevType[DEVICE_TYPE_LEN];				//设备型号
	char szId[DEVICE_INDEX_LEN];					//设备编号
	char szAttrId[DEVICE_SN_LEN];				//设备采集属性编号
	char szSecureTime[20];			//布防时间
	char szStandardValue[60];		//正常值范围
	char szPrivateAttr[255];		//属性私有参数
	char szV_DevType[DEVICE_TYPE_LEN];			//虚拟设备类型
	char szV_Id[DEVICE_INDEX_LEN];					//虚拟设备编号
	char szV_AttrId[DEVICE_SN_LEN];				//虚拟设备属性编号
	char cIsShow;					//是否是主属性
	char szS_define[128];
	char szAttr_Name[20];
	char cStatus;
	char szValueLimit[64];
}StrNetDevAttrUpdate, *PStrNetDevAttrUpdate;

//2006 设备动作属性添加/修改
typedef struct _StrNetDevActionUpdate{      
	char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
	char szDevType[DEVICE_TYPE_LEN];				//设备型号
	char szId[DEVICE_INDEX_LEN];					//设备编号
	char szActionId[DEVICE_ACTION_SN_LEN];				//设备采集属性编号
	char szActionName[20];			//布防时间
	char cStatus;		
	char szPrivateAttr[255];
	char szVDevType[DEVICE_TYPE_LEN];
	char szVDevId[DEVICE_INDEX_LEN];
	char szVActionId[DEVICE_ACTION_SN_LEN];
}StrNetDevActionUpdate, *PStrNetDevActionUpdate;

//2007 设备采集属性删除
typedef struct _StrNetDevAttrDel{      
	char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
	char szDevType[DEVICE_TYPE_LEN];				//设备型号
	char szId[DEVICE_INDEX_LEN];					//设备编号
	char szAttrId[DEVICE_SN_LEN];				//设备采集属性编号
}StrNetDevAttrDel, *PStrNetDevAttrDel;

//2004 设备联动配置
typedef struct _StrNetDevAttrAction{      
	char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
	char szDevType[DEVICE_TYPE_LEN];				//设备型号
	char szId[DEVICE_INDEX_LEN];					//设备编号
	char szAttrId[DEVICE_ACTION_SN_LEN];				//设备采集属性编号
	char ACTIONS[1024];				//联动配置
}StrNetDevAttrAction, *PStrNetDevAttrAction;

//2008/2009  定时任务添加/修改
typedef struct _StrNetTimeTaskUpdate{
    char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
    char szSeq[10];                 //唯一键
	char szId[DEVICE_ID_LEN];					//设备编号
	char szSn[DEVICE_ACTION_SN_LEN];				    //设备动作编号
    char szTime[12];                 //定时时间---lsd改为12位
    char szValue[256];              //动作参数
    char szMemo[128];               //动作说明
}StrNetTimeTaskUpdate, *PStrNetTimeTaskUpdate;

//2010  定时任务删除
typedef struct _StrNetTimeTaskDel{
    char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
    char szSeq[10];                 //唯一键
}StrNetTimeTaskDel, *PStrNetTimeTaskDel;

//2011/2012 设备联动属性添加/修改
typedef struct _StrNetDevAgentUpdate{      
	char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
	char szSeq[10];					//联动编号
	char szDevType[DEVICE_TYPE_LEN];				//设备型号
	char szId[DEVICE_INDEX_LEN];					//设备编号
	char szAttrId[DEVICE_SN_LEN];				//设备采集属性编号
	char szCondition[128];			//联动触发条件
	char szValibtime[30];			//联动有效时段
	char szInterval[4];					//联动间隔
	char szTimes[4];					//联动次数
	char szActDevType[DEVICE_TYPE_LEN];			//联动设备类型
	char szActDevId[DEVICE_INDEX_LEN];				//联动设备编号
	char szActActionId[DEVICE_ACTION_SN_LEN];			//联动设备动作编号
	char szActValue[256];			//联动时填充的内容
	char cType;						//联动类型：0:采集联动，1:离线联动
}StrNetDevAgentUpdate, *PStrNetDevAgentUpdate;

//3001设备布防撤防
typedef struct _StrNetDevDefenceSet{      
	char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
	char szDevType[DEVICE_TYPE_LEN];				//设备型号
	char szId[DEVICE_INDEX_LEN];					//设备编号
	char cType;						//布防、撤防配置
}StrNetDevDefenceSet, *PStrNetDevDefenceSet;

//3003设备同步指令
typedef struct _StrNetDevSyn{      
	char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
}StrNetDevSyn, *PStrNetDevSyn;

//动作下发
typedef struct _StrNetCmd{      
	char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
	char szDevType[DEVICE_TYPE_LEN];				//设备型号
	char szId[DEVICE_INDEX_LEN];					//设备编号
	char szActionId[DEVICE_ACTION_SN_LEN];				//设备动作属性编号
	char szOprator[10];				//设备操作员
	char szActionValue[256];				//动作值
}StrNetCmd, *PStrNetCmd;
/*动作下发
typedef struct _StrNetSCHSysCfg{      
	char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
	char szTimeStatusChange[10];	//
	char szTimeOutDoor[10];			//
	char szCountIndoor[10];			//
	char szTimeIndoor[10];			//
}StrNetSCHSysCfg, *PStrNetSCHSysCfg;*/

typedef struct _MSG_NET_UPLOAD
{
	char szReserve[20];             //保留字
	char szStatus[4];				//命令发送状态
	char szDeal[4];					//处理指令
	char szData[2020];				//数据
}MSG_NET_UPLOAD, *PMSG_NET_UPLOAD;
#pragma pack()

void Net_App_Resp_Proc(void* data, char* outBuf, int outLen,  unsigned int seq);
int Net_Dev_Status_Query(void * userData);
int Net_Dev_Paras_Query(void * msg);
int Net_Dev_Modes_Query(void * msg);

int Net_Dev_Query(const char* inBuf, int inLen, char* outBuf, int& outLen);
int Net_Dev_Real_Status_Query(const char* inBuf, int inLen, char* outBuf, int& outLen);

//设备添加
int Net_Dev_Add(const char* inBuf, int inLen, char* outBuf, int& outLen);

//设备更新
int Net_Dev_Update(const char* inBuf, int inLen, char* outBuf, int& outLen);

//设备删除
int Net_Dev_Del(const char* inBuf, int inLen, char* outBuf, int& outLen);

//设备采集属性修改
int Net_Dev_Attr_Update(const char* inBuf, int inLen, char* outBuf, int& outLen);
//设备动作属性修改
int Net_Dev_Action_Update(const char* inBuf, int inLen, char* outBuf, int& outLen);

//定时任务添加
int Net_Time_Task_Add(const char* inBuf, int inLen, char* outBuf, int& outLen);

//定时任务修改
int Net_Time_Task_Update(const char* inBuf, int inLen, char* outBuf, int& outLen);

//定时任务删除
int Net_Time_Task_Del(const char* inBuf, int inLen, char* outBuf, int& outLen);

int Net_Dev_Agent_Add(const char* inBuf, int inLen, char* outBuf, int& outLen);
int Net_Dev_Agent_Update(const char* inBuf, int inLen, char* outBuf, int& outLen);
int Net_Dev_Agent_Del(const char* inBuf, int inLen, char* outBuf, int& outLen);


//布防撤防配置
//int Net_Dev_Defence_Update(const char* inBuf, int inLen, char* outBuf, int& outLen);

//动作下发
int Net_Dev_Action(const char* inBuf, int inLen, unsigned int seq, int actionSource);

bool Net_Up_Msg(int upCmd, char* buf, int bufLen);
int Net_Dev_Syn(const char* inBuf, int inLen, char* outBuf, int& outLen);
int Net_Dev_Attr_Syn(const char* inBuf, int inLen, char* outBuf, int& outLen);
int Net_Dev_Act_Syn(const char* inBuf, int inLen, char* outBuf, int& outLen);


int Net_GSM_READ_IMSI(const char* inBuf, int inLen, char* outBuf, int& outLen);
#endif
