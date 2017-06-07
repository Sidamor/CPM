#ifndef _DEFINE_APP_H_
#define _DEFINE_APP_H_

#include "Shared.h"

enum _CPM_DATA_TYPE
{
	SHORT = 1,
	INTEGER ,
	FLOAT,
	DOUBLE,
	BYTES = 5,
	CHAR,
	CHARS,
	BCD,
	BCD_2 = 9,
	UCHAR,
	BIT
};

#define DEVICE_DETAIL_MAX		256
#define DEVICE_MODE_MAX		    1024
#define DEVICE_ATTR_MAX			2048//V1.0.1.2


#define	 DEVICE_TYPE_LEN		(6)
#define	 DEVICE_TYPE_LEN_TEMP   (7)

#define	 DEVICE_INDEX_LEN		(4)
#define	 DEVICE_INDEX_LEN_TEMP	(5)

#define  DEVICE_ID_LEN			(10)	//设备编号长度
#define  DEVICE_ID_LEN_TEMP		(11)	//设备编号长度

#define  DEVICE_SN_LEN          (4)	//属性编号长度
#define  DEVICE_SN_LEN_TEMP     (5)	//属性编号长度


#define  DEVICE_CUR_ACTION_LEN          (4)	//当前动作值长度
#define  DEVICE_CUR_ACTION_LEN_TEMP     (5)	//属性编号长度

#define  DEVICE_ACTION_SN_LEN          (8)	//属性编号长度
#define  DEVICE_ACTION_SN_LEN_TEMP     (9)	//属性编号长度

#define  DEVICE_ATTR_ID_LEN     (14)	//属性编号长度
#define  DEVICE_ATTR_ID_LEN_TEMP  (15)	//属性编号长度

#define  DEVICE_ACT_ID_LEN		(18)	//动作属性编号长度
#define  DEVICE_ACT_ID_LEN_TEMP	(19)	//动作属性编号长度

#define  DEVICE_MODE_LEN		(4)	//设备动作模式编号长度
#define  DEVICE_MODE_LEN_TEMP	(5)	//设备动作模式编号长度
#define  DEVICE_MODEID_LEN		(14)	//设备动作模式编号长度
#define  DEVICE_MODEID_LEN_TEMP	(15)	//设备动作模式编号长度

#define  CPM_CPM_ID				"001102"
#define  CPM_GATEWAY_ID			"0011010001"
#define  GSM_SMS_DEV			"001103"
#define  GSM_CALL_DEV			"001104"


#define  DEV_TYPE_CNOOC_DEAL		"032"
#define  DEV_TYPE_CNOOC_DEAL_ATTID	"0001"

#define  DEV_TYPE_READER		"029101"
#define  DEV_TYPE_READER_ATTID	"0001"

#define	 ACTION_DEFENCE			"0001"		//布防/撤防模式
#define	 ACTION_SET_DEFENCE		"00010001"		//布防动作
#define	 ACTION_REMOVE_DEFENCE	"00010002"	//撤防动作

#define ONE_DAY_SECONDS (86400)    //一天的秒数

#define DB_TODAY_PATH		"/home/CPM/tmp.db"//临时数据库
#define DB_HISTORY_PATH		"/home/CPM/cpm.db"//历史数据库
#define DB_SAMPLE_PATH		"/home/CPM/sample.db"//"标本数据库"

//队列 内存定义
typedef enum{	// 存取通道号定义
	MUCB_IF_NET_CLI_RCV=0,
	MUCB_IF_NET_CLI_SEND,

	MUCB_IF_NET_SVR_RCV_BOA,
	MUCB_IF_NET_SVR_SEND_BOA,

	MUCB_IF_NET_SVR_RCV_TMN,
	MUCB_IF_NET_SVR_SEND_TMN,
		
	MUCB_DEV_485_1,		//
	MUCB_DEV_485_2,		//
	MUCB_DEV_485_3,		//
	MUCB_DEV_485_4,		//
	MUCB_DEV_485_5,		//
	MUCB_DEV_485_6,		//
	MUCB_DEV_485_7,		//
	MUCB_DEV_485_8,		//
	MUCB_DEV_ADS,		//
	MUCB_DEV_GSM,		//
	MUCB_DEV_NET,		//

	MUCB_DEV_ANALYSE,	//	
    MUCB_DEV_MSGCTRL_BOA,
	MUCB_DEV_MSGCTRL_TMN,

	MUCB_DEV_SMS_RECV,
	MUCB_DEV_SMS_SEND,

	//
	MUCB_DEV_NET_ACTION,
	MUCB_DEV_TRANSPARENT,
	MUCB_MAX
}TBLMUCBEnum;

typedef enum{
	MOD_CTRL,

	MOD_IF_CLI_NET,                 //
	MOD_IF_CLI_CTRL,
	
	MOD_IF_SVR_NET_BOA,
	MOD_IF_SVR_CTRL_BOA,

	MOD_IF_SVR_NET_TMN,
	MOD_IF_SVR_CTRL_TMN,
	//
	MOD_ACTION_485_1,
	MOD_ACTION_485_2,
	MOD_ACTION_485_3,
	MOD_ACTION_485_4,
	MOD_ACTION_485_5,
	MOD_ACTION_485_6,
	MOD_ACTION_485_7,
	MOD_ACTION_485_8,
	MOD_ACTION_ADS,
	MOD_ACTION_GSM,
	MOD_ACTION_NET,

	MOD_DEV_ANALYSE,
    MOD_DEV_MSGCTRL_BOA,
	MOD_DEV_MSGCTRL_TMN,

	MOD_DEV_SMS_RECV,
	MOD_DEV_SMS_SEND,
	
	//
	MOD_DEV_NET_ACTION,
	
	MOD_DEV_TRANSPARENT,		//透明传输业务处理队列,

	SYS_MOD_MAX
}SysModType;

//hash节点，比较指令
typedef enum{
	COMPARE_TYPE		= 0x01,
	COMPARE_MODE		= 0x02,
	COMPARE_ID			= 0x03,
	COMPARE_tid			= 0x04,
	COMPARE_TID			= 0x05,
    COMPARE_POSITION    = 0x06
}HASH_COMPARE_CMD;

//CPM到平台
#define CMD_DEVICE_STATUS_UPDATE      (1000)         //设备状态变化上传
#define CMD_ENVIRON_DATA_UPDATE       (1001)         //环境数据上传
#define CMD_YKT_DATA_UPDATE           (1002)         //YKT数据上传
#define CMD_ACT_UPLOAD                (1003)         //联动上传
#define CMD_ALARM_UPLOAD              (1004)         //GSM告警数据上传
#define CMD_DEVICE_ATTR_STATUS_UP     (1005)         //设备采集属性状态变化上传
#define CMD_DEVICE_INFO_UP			  (1006)         //设备信息上传
#define CMD_DEVICE_ATTR_UP			  (1007)         //设备采集属性信息上传
#define CMD_DEVICE_ACT_UP			  (1008)         //设备动作属性信息上传
#define CMD_TRANSPARENT_DATA_UP	      (1011)         //中海油零售数据上传

//web/平台到CPM
#define CMD_DEVICE_QUERY			(2000)         //设备状态查询
#define CMD_DEVICE_STATUS_QUERY		(2001)		   //设备实时状态查询
#define CMD_DEVICE_ADD				(2002)         //设备添加
#define CMD_DEVICE_UPDATE			(2003)         //设备修改
#define CMD_DEVICE_DEL				(2004)         //设备删除

#define CMD_DEVICE_PRIATTR_UPDATE	(2005)         //设备采集属性修改
#define CMD_DEVICE_ACTION_UPDATE	(2006)         //设备采集属性删除

#define CMD_ONTIME_TASK_ADD         (2008)         //定时任务增加
#define CMD_ONTIME_TASK_UPDATE      (2009)         //定时任务修改
#define CMD_ONTIME_TASK_DEL         (2010)         //定时任务删除


#define CMD_DEVICE_AGENT_ADD        (2011)         //设备联动添加
#define CMD_DEVICE_AGENT_UPDATE     (2012)         //设备联动修改
#define CMD_DEVICE_AGENT_DEL        (2013)         //设备联动删除


#define CMD_DEVICES_STATUS_QUERY	(2020)         //设备状态查询
#define CMD_DEVICES_PARAS_QUERY		(2021)         //属性数据查询
#define CMD_DEVICES_MODES_QUERY		(2022)         //模式数据查询

//远程控制
#define CMD_DEVICE_DEFENCE_UPDATE	(3001)         //布防撤防配置
#define CMD_ACT_SEND				(3002)         //动作下发

#define CMD_DEVICE_SYN				(3003)         //设备同步
#define CMD_DEVICE_ATTR_SYN			(3004)         //设备属性同步
#define CMD_DEVICE_ACT_SYN			(3005)         //设备属性同步


#define CMD_SYS_CONFIG_SCH			(3006)         //SCH系统参数配置

#define CMD_GSM_READ_IMSI			(3007)         //GSM读取卡IMSI号


//网络型终端到CPM
#define CMD_TERMINAL_DATA_SUBMIT			(4001)	//数据不需要回应
#define CMD_TERMINAL_DATA_SUBMIT_NEEDRESP	(4011)	//数据需要回应

//CPM到网络型终端
#define CMD_TERMINAL_CTRL			(4002)			//控制

#pragma pack(1)

/************************************************************************/

#define MAX_PACKAGE_LEN_TERMINAL		512
#define MAX_PACKAGE_LEN_NET				2048

//解析模式选择
#define DECODE_TYPE_RDFORMAT     (1)      //以读取值解析格式解析
#define DECODE_TYPE_WDFORMAT     (2)      //以写入值解析格式解析

enum _INTERFACE_TYPE
{
	TYPE_INTERFACE_RS485 = 1,
	TYPE_INTERFACE_AD_A,
	TYPE_INTERFACE_AD_V,
	TYPE_INTERFACE_DI,
	TYPE_INTERFACE_DO,
	TYPE_INTERFACE_AV_OUT,
	TYPE_INTERFACE_GSM,
	TYPE_INTERFACE_RF,
	TYPE_INTERFACE_NET
};

//CPM 设备接口定义
enum _INTERFACE_CPM_
{
	//232串口接口
	INTERFACE_RS232 = 0,

	//485接口
	INTERFACE_RS485_1 = 1,    //485 1号口        *****************************************
	INTERFACE_RS485_2,        //485 2号口        *  1号口  *  3号口  *  5号口  *  7号口  *
							  //                 *****************************************
	INTERFACE_RS485_3,       //485 3号口        *  2号口  *  4号口  *  6号口  *  8号口  *
	INTERFACE_RS485_4,       //485 4号口        *****************************************
	INTERFACE_RS485_5,       //485 5号口     485(1) ->ttyS3    485(8) ->SPI1.1 (serial0)   485(7) ->SPI1.2 (serial0)
	INTERFACE_RS485_6,       //485 6号口     485(2) ->ttyS2    485(4) ->SPI1.1 (serial1)   485(3) ->SPI1.2 (serial1)
	INTERFACE_RS485_7,       //485 7号口                       485(6) ->SPI1.1 (serial2)   485(5) ->SPI1.2 (serial2)
	INTERFACE_RS485_8,       //485 8号口

	//模拟量输入A组
	INTERFACE_AD_A0 = 9,
	INTERFACE_AD_A1,
	INTERFACE_AD_A2,
	INTERFACE_AD_A3,

	//模拟量输入B组
	INTERFACE_AD_B0,
	INTERFACE_AD_B1,
	INTERFACE_AD_B2,
	INTERFACE_AD_B3,

	//开关量输入A组
	INTERFACE_DIN_A0 = 17,
	INTERFACE_DIN_A1,
	INTERFACE_DIN_A2,
	INTERFACE_DIN_A3,         //20

	//开关量输入B组
	INTERFACE_DIN_B0,
	INTERFACE_DIN_B1,
	INTERFACE_DIN_B2,
	INTERFACE_DIN_B3,

	//开关量输出上排
	INTERFACE_DOUT_A2 = 25,
	INTERFACE_DOUT_A4,
	INTERFACE_DOUT_B2,
	INTERFACE_DOUT_B4,

	//开关量输出下排
	INTERFACE_DOUT_A1,
	INTERFACE_DOUT_A3,
	INTERFACE_DOUT_B1,
	INTERFACE_DOUT_B3,

	//继电器电源输出
	INTERFACE_AV_OUT_1 = 33,

	INTERFACE_GSM = 34,
	//RF-433接口
	INTERFACE_RF433 = 35,

	//家校通
	INTERFACE_SHC = 40,

	//网络数据
	INTERFACE_NET_INPUT = 50,

	MAX_INTERFACE_COUNT

};

enum DEVICE_STATUS
{
	DEVICE_NOT_USE,		//设备撤防
	DEVICE_OFFLINE,		//设备离线
	DEVICE_EXCEPTION,	//设备异常
	DEVICE_USE,			//设备布防
	DEVICE_ONLINE		//设备在线
};
enum ACTION_SOURCE
{
	ACTION_SOURCE_SYSTEM,		//系统联动
	ACTION_SOURCE_NET_CPM,		//CPM嵌入式控制指令
	ACTION_SOURCE_NET_PLA,		//平台控制指令
	ACTION_SOURCE_GSM,			//从GSM发起控制
    ACTION_SOURCE_ONTIME,       //定时任务
	ACTION_TO_GSM,				//发往GSM的动作
	ACTION_RESUME				//模式后续动作
};
enum ALARM_LEVEL
{
	ALARM_SERIOUS = 1,				//严重告警
	ALARM_GENERAL,				//普通告警
	ALARM_ATTENTION,			//一般提示
	ALARM_UNKNOW				//未知告警
};

enum SYS_ACTION
{
	SYS_ACTION_SET_DEFENCE = 1,		//布防动作
	SYS_ACTION_REMOVE_DEFENCE = 2		//撤防动作
};

enum SYS_STATUS
{
	SYS_STATUS_SUCCESS						= 0,	//	成功
	SYS_STATUS_ILLEGAL_CHANNELID			= 1001,	//	通道不合法
	SYS_STATUS_DEVICE_TYPE_NOT_SURPORT		= 2001,	//	该类设备不支持
	SYS_STATUS_DEVICE_NOT_EXIST				= 2002,	//	设备不存在
	SYS_STATUS_DEVICE_COUNT_MAX				= 2003,	//	设备条数超过上限
	SYS_STATUS_DEVICE_ATTR_OFFLINE			= 2004,	//	设备属性离线
	SYS_STATUS_DEVICE_ATTR_ONLINE			= 2005,	//	设备属性在线
	SYS_STATUS_DEVICE_DEV_OFFLINE			= 2006,	//	设备离线
	SYS_STATUS_DEVICE_DEV_ONLINE			= 2007,	//	设备在线
	SYS_STATUS_DEVICE_ATTR_MAX				= 2008,	//	设备属性条数超过上限
	SYS_STATUS_DEVICE_ALREADY_EXIST			= 2010,	//	设备属性已存在
	SYS_STATUS_DEVICE_ACTION_NOT_EXIST		= 2011,	//	设备动作属性不存在
	SYS_STATUS_DEVICE_ANALYSE_NOT_EXIST		= 2012,	//	联动设备不存在
	SYS_STATUS_DEVICE_ANALYSE_ACTION_FULL	= 2013,	//	设备联动属性满
	SYS_STATUS_DEVICE_ACTION_NOT_USE		= 2014,	//	设备动作未启用
	SYS_STATUS_DEVICE_SINGLE_AGENT_MAX		= 2015,	//	设备单属性联动条数超过上限
	SYS_STATUS_DEVICE_AGENT_MAX				= 2016,	//	设备总联动条数超过上限
	SYS_STATUS_DEVICE_ACTION_MAX			= 2017,	//	设备总联动条数超过上限

	SYS_STATUS_SUBMIT_SUCCESS				= 3000,	//	提交成功
	SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP		= 3001,	//	提交成功，需等待回应
	SYS_STATUS_SUBMIT_SUCCESS_IN_LIST		= 3002,	//	已插入到执行队列
	SYS_STATUS_CREATE_THREAD_FAILED			= 3003,	//	线程启动失败
	SYS_STATUS_FAILED						= 3006,	//	失败
	SYS_STATUS_FAILED_NO_RESP				= 3007,	//	失败,无需回应
	SYS_STATUS_DEVICE_REMOVE_DEFENCE		= 3008,	//	失败,设备已撤防
	SYS_STATUS_DEVICE_COMMUNICATION_ERROR	= 3009,	//	通信异常

	SYS_STATUS_TASK_INFO_NOT_EMPTY			= 3010, //  任务表有该记录
	SYS_STATUS_AGENT_INFO_NOT_EMPTY			= 3011, //  联动表有该记录
	SYS_STATUS_ATTR_USED_BY_OTHER			= 3012, //  联动表有该记录
	SYS_STATUS_ROLE_NOT_EMPTY				= 3013, //  防区表有该记录

	SYS_STATUS_ACTION_ENCODE_ERROR			= 3020,	//	写数据组码错误
	SYS_STATUS_ACTION_DECODE_ERROR		    = 3021, //	写数据解码错误
	SYS_STATUS_ACTION_DECODE_FORMAT_ERROR	= 3022,	//	写数据回应解码格式配置错误
	SYS_STATUS_ACTION_COMPARE_FORMAT_ERROR	= 3023,	//	写数据回应比较表格式配置错误
	SYS_STATUS_ACTION_RESP_FAILED			= 3024,	//	写数据物联终端回应失败
	SYS_STATUS_ACTION_RESP_FORMAT_ERROR		= 3025,	//	写数据物联终端回应数据格式有误
	SYS_STATUS_ACTION_WFORMAT_ERROR			= 3026,	//	写格式错误

	SYS_STATUS_ACION_RESUME_CONFIG_ERROR	= 3030, //  后续动作配置错误
	SYS_STATUS_ACION_RESUME_ERROR			= 3031, //  执行后续动作失败

	SYS_STATUS_DATA_DEOCDE_FAIL				= 3040,	//	解析失败
	SYS_STATUS_DATA_DECODE_NORMAL			= 3041,	//	解析成功，正常数据
	SYS_STATUS_DATA_DECODE_UNNOMAL_THROW	= 3042,	//	解析成功，异常数据，丢弃
	SYS_STATUS_DATA_DECODE_UNNOMAL_AGENT	= 3043,	//	解析成功，异常数据，提交Agent

	SYS_STATUS_COMM_PARA_ERROR				= 3100,	//	串口参数错误
	SYS_STATUS_COMM_BAUDRATE_ERROR			= 3101,	//	串口波特率错误
	SYS_STATUS_OPEN_COMM_FAILED				= 3102,	//	打开串口失败
	SYS_STATUS_SEND_COMM_FAILED				= 3103,	//	发送串口数据失败
	SYS_STATUS_RECV_COMM_FAILED				= 3104,	//	接收串口数据失败

	SYS_STATUS_GSM_BUSY						= 4001,	//  GSM繁忙
	SYS_STATUS_GSM_PLAY_SOUND_ERROR			= 4002,	//  GSM放音失败
	SYS_STATUS_GSM_CALL_NO_CARRIER			= 4003,	//  GSM无信号
	SYS_STATUS_GSM_CALL_BUSY				= 4004,	//  对方正忙
	SYS_STATUS_GSM_CALL_NO_DIALTONE			= 4005,	//  无拨号音
	SYS_STATUS_GSM_CALL_BARRED				= 4006,	//  拨号被禁
	SYS_STATUS_GSM_SENDLIST_EXCEPTION		= 4007,	//  GSM发送队列异常
	
	SYS_STATUS_TASK_TIME_FORMAT_ERROR		= 9980,	//	定时任务时间格式错误
	SYS_STATUS_TASK_TIME_MAX				= 9981,	//	定时任务总条数超过上限
	SYS_STATUS_INTERFACE_NOT_USED			= 9990, //  接口未启用
	SYS_STATUS_DB_LOCKED_ERROR				= 9991,	//	数据库被锁
	SYS_STATUS_CACULATE_MODE_ERROR			= 9992,	//	计算方式错误
	SYS_STATUS_INSERT_DB_FAILED				= 9993,	//	插入到数据库失败
	SYS_STATUS_SEQUENCE_ERROR				= 9994,	//	流水号异常
	SYS_STATUS_FORMAT_ERROR					= 9995,	//	格式错误
	SYS_STATUS_ILLEGAL_REQUEST				= 9996,	//	非法请求	
	SYS_STATUS_TIMEOUT						= 9997,	//	处理超时
	SYS_STATUS_SYS_BUSY						= 9998,	//	系统繁忙	
	SYS_STATUS_UNKONW_ERROR					= 9999	//	未知错误
};
/************************************************************************/
//--------------------------------------------------------------------------------网络通信消息体

typedef struct _MsgLogin{      //网络登录包
	BYTE szLoginId[10];             
	BYTE szTime[14];
	BYTE szMd5[32];
}MsgLogin, *PMsgLogin;

typedef struct _MsgPlDevAdd{      //2001 设备添加
    char szDevId[DEVICE_ID_LEN];              //设备编号
    char szCName[30];						  //设备名称
    char szUpper[DEVICE_ID_LEN];              //上级设备
}MsgPlDevAdd, *PMsgPlDevAdd;

typedef struct _MsgPlDevDel{      //2003 设备删除
    char szDevId[DEVICE_ID_LEN];              //设备编号
}MsgPlDevDel, *PMsgPlDevDel;

typedef struct _MsgPlActUpdate{   //2004 联动配置更新
    char szDevId[DEVICE_ID_LEN];
    char szActUpdate[1024];
}MsgPlActUpdate, *PMsgPlActUpdate;

typedef struct _MsgPlPriAttr{     //2005 设备私有属性更新
    char szDevId[DEVICE_ID_LEN];  //设备标号
    char szCtype[2];              //应用类型
    char szUpChannel[8];          //通道号
    char szSelfId[20];            //子通道号
    char szSecureTime[20];        //时间
    char szStandardValue[60];     //正常值范围
    char szPrivateAttr[255];      //私有参数
}MsgPlPriAttr, *PMsgPlAttr;

typedef struct _MsgPlDefen{       //3001 设备布防撤防更新
    char szStatus;
    char devIds[256];
}MsgPlDefen, *PMsgPlDefen;

typedef struct _MsgPlActSend{     //3002 动作下发
    char szOprator[10];           //操作用户
    char szDevId[DEVICE_ID_LEN];  //设备号
    char szActId[60];             //动作指令
    char szActValue[128];         //动作值
}MsgPlActSend, *PMsgPlActSend;

typedef struct _MsgRecvFromPl{
    char szReserve[20];
    char szStatus[4];
    char szDeal[4];
    union {
        MsgPlDevAdd      stPlDevAdd;
        MsgPlDevDel      stPlDevDel;
        MsgPlActUpdate   stPlActUpdate;
        MsgPlPriAttr     stPlPriAttr;
        MsgPlDefen       stPlDefen;
        MsgPlActSend     stPlActSend;
    };
}MsgRecvFromPl, *PMsgRecvFromPl;

typedef struct _MsgRespToPl{
    char szReserve[20];
    char szStatus[4];
    char szDeal[4];
}MsgRespToPl, *PMsgRespToPl;


typedef struct{
	char szApmId[11]; 
	char szName[6];
	char szPwd[6];
	char szIP[15];
	BYTE btStatus;
	int nClientId;	
}ApmInfo;


typedef struct{
	char szIniFileName[128];	//配置文件或数据库路径
	char szModName[30];			//模块名
	BYTE pid;               //通道号
	BYTE self;				//模块标识
	BYTE ctrl;				//控制模块
	BYTE remote;			//远端模块
	TKU32 unMemHdl;				//接收线程申请内存所需的句柄
	TKU32 unMemHdlSend;			//发送线程申请内存所需的句柄
}ModInfo;

//--------------------------------------------------------------------------------协议通信解析结构体
typedef struct _NET_DEVICE_INFO_UPDATE
{
	char Id[DEVICE_ID_LEN_TEMP];		//设备编号(8位)
	char Upper_Channel[3];              //设备上级通道号
	char Self_Id[21];                   //通信ID
	char PrivateAttr[256];				//设备私有属性
} NET_DEVICE_INFO_UPDATE, *PNET_DEVICE_INFO_UPDATE;


typedef struct _NET_DEVICE_ATTR_INFO_UPDATE
{
	char Id[DEVICE_ID_LEN_TEMP];		//设备编号(6位)
	char AttrId[DEVICE_SN_LEN_TEMP];						//设备采集属性编号
	char SecureTime[21];                //布防时间
	char StandardValue[61];             //标准值
	char PrivateAttr[256];				//私有参数
	char szV_Id[DEVICE_ID_LEN_TEMP];
	char szV_AttrId[DEVICE_SN_LEN_TEMP];
	char cStaShow;
	char szS_define[129];
	char szAttr_Name[21];
	char cStatus;
	char szValueLimit[65];

} NET_DEVICE_ATTR_INFO_UPDATE, *PNET_DEVICE_ATTR_INFO_UPDATE;

typedef struct _DEVICE_DETAIL_STATUS
{
	char Id[DEVICE_ID_LEN_TEMP];                        //设备编号(8位)
	char szCName[128];					//设备名
	char Status;                        //当前状态
	char Show[56];						//状态显示
} DEVICE_DETAIL_STATUS, *PDEVICE_DETAIL_STATUS;

typedef struct _DEVICE_DETAIL_PARAS
{
	char Id[DEVICE_ID_LEN_TEMP];                        //设备编号(10位)
	char ParaValues[MAXMSGSIZE];
} DEVICE_DETAIL_PARAS, *PDEVICE_DETAIL_PARAS;

typedef struct _DEVICE_DETAIL_MODES
{
	char Id[DEVICE_ID_LEN_TEMP];                        //设备编号(10位)
	char ModeId[DEVICE_ACTION_SN_LEN_TEMP];			  //模式编号+值编号
	char ParaValues[MAXMSGSIZE];
} DEVICE_DETAIL_MODES, *PDEVICE_DETAIL_MODES;

typedef struct _DEVICE_DETAIL
{
	char Id[DEVICE_ID_LEN_TEMP];                        //设备编号(6位)
	char szCName[128];					//设备名
	char Status;                        //当前状态
	char Show[56];						//状态显示
	char ParaValues[1024];
} DEVICE_DETAIL, *PDEVICE_DETAIL;

typedef struct _PARA_INFO	//参数数据
{
	char szId[3];			//系统编号
	char szCname[13];		//名称，如温度，湿度等
	char szUnit[13];		//单位，如℃等
	char szMemo[129];		//备注
}PARA_INFO, *PPARA_INFO;

//G, E, A 通信接口
typedef struct _AgentInfo{  
	int	 iSeq;					//联动编号
	char szDevId[DEVICE_ID_LEN_TEMP];				//联动编号
	char szAttrId[DEVICE_SN_LEN_TEMP];				//设备采集属性编号
	char szCondition[129];			//联动触发条件
	char szValibtime[31];			//联动有效时段
	int  iInterval;					//联动间隔
	int  iTimes;					//联动次数
	char szActDevId[DEVICE_ID_LEN_TEMP];				//联动设备编号
	char szActActionId[DEVICE_ACTION_SN_LEN_TEMP];			//联动设备动作编号
	char szActValue[257];			//联动时填充的内容
	char cType;						//联动类型：0:采集联动，1:离线联动
}AgentInfo, *PAgentInfo;

//--------------------------------------------------------------------------------设备表结构体
typedef struct _LAST_VALUE_INFO          //
{
	char DevAttrId[DEVICE_ATTR_ID_LEN_TEMP];         //设备编号
	char Type[5];           //数据类型
	struct timeval LastTime;            //上次采样时间
	int	 VType;             //数值种类，如浮点型，字符串型等
	char Value[MAX_PACKAGE_LEN_TERMINAL];        //采集数据
	int  DataStatus;		//数据状态
	char szLevel[5];
	char szDescribe[257];
	char szValueUseable[MAX_PACKAGE_LEN_TERMINAL];
} LAST_VALUE_INFO, *PLAST_VALUE_INFO;

//采集属性结构
typedef struct _DEVICE_INFO_POLL
{
	char Id[DEVICE_ATTR_ID_LEN_TEMP];   //设备编号(10位) + 轮询SN(4位)
	char Upper_Channel[9];              //设备上级通道号
	char Self_Id[22];                   //通信ID
	char R_Format[128];                 //读格式,lhy
	char Rck_Format[128];               //读校验,lhy
	char RD_Format[128];                //读分析,lhy
	char RD_Comparison_table[512];      //读查表,lhy
	char szAttrName[21];				//属性名称
	char cIsNeedIdChange;				//需要将值转换为用户编号

	char cBeOn;							//属性启用状态V10.0.0.8

	unsigned int BaudRate;				//波特率
	unsigned int databyte;              //数据位
	unsigned int stopbyte;              //停止位
	unsigned int timeOut;				//读串口超时时间
	char checkMode[10];                 //串口数据校验方式

	char V_Id[DEVICE_ATTR_ID_LEN_TEMP];	//虚拟属性编号
	char cStaShow;
	char szS_Define[128];				//状态转换定义
	char szStandard[60];				//标准范围
	char cLedger;						//统计标记

	char szValueLimit[65];				//lsd值域范围
	int Range_Type;						//值域类型
	float Range_1;						//值域边界1
	float Range_2;						//值域边界2
	int Range_Agent;					//超出值域处理办法（0不记录，1记录-交予agent）
	int offLineTotle;					//lsd一共几次离线

	int isSOIF;							//lsd是否使用定制处理库 0,不使用;1,使用
	int isTransparent;					//lsd是否数据透明传输	0,不使用;1,使用

	char PreId[DEVICE_ATTR_ID_LEN_TEMP];					//读取方式相同的上一设备属性
	char NextId[DEVICE_ATTR_ID_LEN_TEMP];					//读取方式相同的下一设备属性

	char ChannelPreId[DEVICE_ATTR_ID_LEN_TEMP];				//通道相同的上一设备属性
	char ChannelNextId[DEVICE_ATTR_ID_LEN_TEMP];			//通道相同的下一设备属性

	unsigned long Frequence;			//轮巡间隔
	unsigned char isUpload;				//是否上传到平台
	int offLineCnt;                    //离线计算
	struct tm startTime;                //布防开始时间
	struct tm endTime;                  //布防结束时间

	char szAttrValueLimit[65];			//lsd网元状态定义
	int	retryCount;
	
	LAST_VALUE_INFO stLastValueInfo;		//最新数据
} DEVICE_INFO_POLL, *PDEVICE_INFO_POLL;

//动作属性结构
typedef struct _DEVICE_INFO_ACT
{
	char Id[DEVICE_ACT_ID_LEN_TEMP];    //设备编号(10位) + 动作ID(8位)
	char Upper[11];						//设备上级编号（ID）
	char Upper_Channel[9];              //设备上级通道号
	char Self_Id[22];                   //通信ID
	char cBeOn;							//启用装填
	char W_Format[128];                 //写格式,lhy
	char Wck_Format[128];               //写校验,lhy
	char WD_Format[128];                //写分析,lhy
	char WD_Comparison_table[512];     //写查表,lhy
	unsigned int BaudRate;				//波特率
	unsigned int databyte;              //数据位
	unsigned int stopbyte;              //停止位
	unsigned int timeOut;				//读串口超时时间
	char checkMode[10];                 //串口数据校验方式
	time_t LastTime;					//上次动作时间
	char szCmdName[21];		
	char ctype2[3];
	char szVActionId[DEVICE_ACT_ID_LEN_TEMP];				//虚拟动作编号
	char flag2;							//是否设置为当前模式
	char flag3;							//是否执行前一模式
} DEVICE_INFO_ACT, *PDEVICE_INFO_ACT;

//--------------------------------------------------------------------------------主控程序与分析程序结构体
//数据采集结果数据
typedef struct _QUERY_INFO_CPM          //传送至解析模块的数据格式
{
	char Cmd;               //若为1 仅作设定设备在线状态用
	char DevId[DEVICE_ATTR_ID_LEN_TEMP];         //设备编号
	char Type[5];           //数据类型
	char Time[21];          //数据采集时间
	int	 VType;             //数值种类，如浮点型，字符串型等
	char Value[MAX_PACKAGE_LEN_TERMINAL];        //采集数据
	int  ValueStatus;		//lsd值域状态，0为正常,1为异常不处理,2为异常且交给agent处理
	int  DataStatus;		//数据状态,1为异常
} QUERY_INFO_CPM, *PQUERY_INFO_CPM;

typedef struct _QUERY_INFO_CPM_EX          //传送至解析模块的数据格式
{
	char Cmd;               //若为1 仅作设定设备在线状态用
	char DevId[DEVICE_ATTR_ID_LEN_TEMP];         //设备编号
	char Type[5];           //数据类型
	char Time[21];          //数据采集时间
	int	 VType;             //数值种类，如浮点型，字符串型等
	char Value[512];        //采集数据
	int  DataStatus;		//数据状态，1为异常
} QUERY_INFO_CPM_EX, *PQUERY_INFO_CPM_EX;

typedef struct _ACTION_MSG
{
	int	 ActionSource;				//来源,0,联动指令；1,嵌入式Web指令；2,平台指令;3,短信指令
	char DstId[DEVICE_ID_LEN_TEMP];				//目的设备
	char ActionSn[DEVICE_ACTION_SN_LEN_TEMP];			//动作属性编号
	char Operator[11];			//操作员编号(联动触发为AUTO)
	char ActionValue[257];		//动作内容值

	char SrcId[DEVICE_ID_LEN_TEMP];				//联动源设备
	char AttrSn[DEVICE_SN_LEN_TEMP];				//联动触发指令
	char SrcValue[128];
	char szActionTimeStamp[21];			//时间戳
	unsigned int	 Seq;		//执行完毕后，用于查找回复路径
}ACTION_MSG, *PACTION_MSG;

typedef struct _ACTION_MSG_RESP_SMS
{
	int	 ActionSource;			//来源,0,联动指令；1,嵌入式Web指令；2,平台指令;3,短信指令
	char DstId[DEVICE_ID_LEN_TEMP];				//目的设备
	char ActionSn[DEVICE_ACTION_SN_LEN_TEMP];			//动作属性编号
	char Operator[11];			//操作员编号(联动触发为AUTO)
	char ActionValue[257];		//动作内容值

	char SrcId[DEVICE_ID_LEN_TEMP];				//联动源设备
	char AttrSn[DEVICE_SN_LEN_TEMP];				//联动触发指令
	char SrcValue[128];
	char szActionTimeStamp[21];			//时间戳
	unsigned int	 Seq;		//执行完毕后，用于查找回复路径
	int Status;
}ACTION_MSG_RESP_SMS, *PCTION_MSG_RESP_SMSG;


typedef struct _ACTION_MSG_WAIT_RESP_HASH
{
	unsigned int Id;			//
	ACTION_MSG   stActionMsg;
}ACTION_MSG_WAIT_RESP_HASH, *PACTION_MSG_WAIT_RESP_HASH;

typedef struct _ACTION_RESP_MSG
{
	TKU32 Seq;        //哈希表编号
	int Status;       //执行状态
}ACTION_RESP_MSG, *PACTION_RESP_MSG;

typedef int (*DeviceInit_CallBack)(void*, DEVICE_INFO_POLL*);

/************************************************************************/
/* 网络传感设备                                                         */
/************************************************************************/
typedef struct _NetDispatch{
	char			szReserve[20];				//Reserve
	char			szDeal[4];					//
	char			szStatus[4];
	char 			szPid[10];					//网络终端编号
	BYTE			szMd5[32];					//MD5
	char			szTid[10];				
	char			szSubId[8];					
	char			szDataLen[4];
	char			szDispatchValue[128];		//ActId[8]/AttrId[4] + DataLen[4] + Data[DataLen]
}NetDispatch, *PNetDispatch;
typedef struct _MsgNetDataSubmit{				//
	char			szReserve[20];				//Reserve
	char			szDeal[4];					//
	char			szStatus[4];
	char 			szPid[10];					//网络终端编号
	BYTE			szMd5[32];					//MD5
	char			szTid[10];					//传感终端编号
	char			szAttrId[DEVICE_SN_LEN];	//属性编号，0000表示全属性，其他表示指定属性
	unsigned int	iDataLen;					//数据长度
	unsigned char	szValue[256];				//数据
}MsgNetDataSubmit, *PMsgNetDataSubmit;
typedef struct _MsgNetDataDeliver{				//
	char			szReserve[20];				//Reserve
	char			szDeal[4];					//
	char			szStatus[4];
	char 			szPid[10];					//网络终端编号
	BYTE			szMd5[32];					//MD5
}MsgNetDataDeliver, *PMsgNetDataDeliver;

typedef struct _MsgNetDataAction{				//
	char			szReserve[20];				//Reserve
	char			szDeal[4];					//
	char			szStatus[4];
	char 			szPid[10];					//网络终端编号
	BYTE			szMd5[32];					//MD5
	char			szTid[10];					//传感终端编号
	char			szAttrId[DEVICE_ACTION_SN_LEN];
	unsigned int	iDataLen;					//数据长度
	unsigned char	szValue[256];				//数据
}MsgNetDataAction, *PMsgNetDataAction;
typedef struct _MsgNetDataActionResp{			//
	char			szReserve[20];				//Reserve
	char			szDeal[4];					//
	char			szStatus[4];
	char 			szPid[10];					//网络终端编号
	BYTE			szMd5[32];					//MD5
}MsgNetDataActionResp, *PMsgNetDataActionResp;

enum _NET_TERMINAL_CMD
{
	NET_TERMINAL_CMD_ACTIVE_TEST		= 1000,		//链路测试
	NET_TERMINAL_CMD_DATA_SUBMIT		= 1001,		//数据上传
	NET_TERMINAL_CMD_ACTION_DELEIVE		= 2001,		//动作指令
	NET_TERMINAL_CMD_DATA_DELEIVE		= 2002		//采集指令
};

typedef struct _DB_OPRATE_FAIL
{
	char szSql[1024];
	time_t tTime;
}DB_OPRATE_FAIL, *PDB_OPRATE_FAIL;

typedef enum _MOD_TYPE
{
	MOD_RS485_1 = 1,
	MOD_RS485_2,
	MOD_RS485_3,
	MOD_RS485_4,
	MOD_RS485_5,
	MOD_RS485_6,
	MOD_RS485_7,
	MOD_RS485_8,
	MOD_AD = 9,
	MOD_GSM = 34,
	MOD_TRANSPARENT = 41,
	MOD_NetTerminal = 50,
	MOD_DB_Insert = 99
}MOD_TYPE;

typedef struct _MODE_CFG
{
	unsigned char Ad;
	unsigned char GSM;
	unsigned char NetTerminal;
	unsigned char Transparent;
	unsigned char RS485_1;
	unsigned char RS485_2;
	unsigned char RS485_3;
	unsigned char RS485_4;
	unsigned char RS485_5;
	unsigned char RS485_6;
	unsigned char RS485_7;
	unsigned char RS485_8;
	unsigned char DB_Insert;
}MODE_CFG, *PMODE_CFG;

typedef struct _RECORD_COUNT_CFG
{
	long User_Max;				//总用户条数				01
	long Dev_Max;				//总设备条数
	long Attr_Max;				//总属性条数
	long Agent_Max;				//总联动条数
	long TimeCtrl_Max;			//总定时任务条数			05
	long Single_Agent_Max;		//单属性联动条数
	long NetSend_Max;			//网络发送队列条数
	long NetRecv_Max;			//网络接收队列条数
	long NetClientSend_Max;		//客户端网络发送队列条数
	long NetClientRecv_Max;		//客户端网络接收队列条数	10
	long RS485_1_Max;			//485-1任务消息条数
	long RS485_2_Max;			//485-2任务消息条数
	long RS485_3_Max;			//485-3任务消息条数
	long RS485_4_Max;			//485-4任务消息条数
	long RS485_5_Max;			//485-5任务消息条数			15
	long RS485_6_Max;			//485-6任务消息条数
	long RS485_7_Max;			//485-7任务消息条数
	long RS485_8_Max;			//485-8任务消息条数
	long ADS_Max;				//ADS任务消息条数
	long GSM_Max;				//GSM任务消息条数			20
	long Action_Max;			//总动作条数				21
}RECORD_COUNT_CFG, *PRECORD_COUNT_CFG;


typedef struct _STR_DEVICE_AD
{
	int iChannel;
	float fValue;
	float fTempValue[512];
	int iTempCount;
	int iErrCount;
}STR_DEVICE_AD, *PSTR_DEVICE_AD;


//GSM

typedef struct _SMS
{
	int	 sn;
	char tel[20];			//手机号
	char content[1025];		//内容
}SMS, *PSMS;



typedef struct _SMS_INFO
{
	int Seq;				//动作源流水号
	int	 sn;				//系统短信流水号
	int  ctype;				//类型，0：语音，1：短信
	char srcDev[DEVICE_ID_LEN_TEMP];        //告警设备编号
	char srcAttr[DEVICE_SN_LEN_TEMP];		//告警属性
	char dstDev[DEVICE_ID_LEN_TEMP];			//目标设备
	char dstAction[DEVICE_ACTION_SN_LEN_TEMP];		//目标动作
	char seprator[11];      //操作者
	char tel[256];			//电话号码
	char szTimeStamp[21];		//时间戳
	int  index;				//短息模块内序号
	char content[512];		//内容
}SMS_INFO, *PSMS_INFO;

typedef struct _GSM_MSG
{
	int iMsgType;//0,GSM接收；1,发送到GSM，需要回应；2,发送到GSM,不需要回应；3，动作回复信息
	union
	{
		SMS_INFO stSmsInfo;
		ACTION_RESP_MSG stActionRespMsg;
		ACTION_MSG	stAction;
		ACTION_MSG_RESP_SMS stRespSMS;
	};
}GSM_MSG, *PGSM_MSG;

#pragma pack()
#endif
