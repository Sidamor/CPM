#ifndef __SHARED_H__
#define __SHARED_H__
//Linux 专用头文件，其他平台使用根据需要修改
#include <string>
#include <stdio.h>
#include <pthread.h>
#include <assert.h> 
#include <fstream>
#include <time.h>
#include <sys/time.h>
//#include <sys/timeb.h>
#include "TkMacros.h"
#include "Md5.h"
using namespace std;

typedef unsigned long DWORD;
typedef unsigned char BYTE;


//常量定义
#define DATALOG_FILE ""
#define DST_ADDR_LEN 30
#define SMS_LEN 160
#define SYS_QUEUE_CNT	        (1024*2) 	//系统中队列数
#define COMBINE_MOD(a, b)	((a << 16)|b) 	//模块的唯一号

#define ICLEN					16

#define PRT_INTERVAL 1	//打印测试信息频率

#pragma pack(1)

//通信协议类型
typedef enum{
	NP_UDP = 0,
	NP_TCP,
	NP_TCPSVR,
	NP_MAX
}NPType;

//接口模块命令信息分析结果
typedef enum{
	CODEC_CMD = 0,  //收到新命令
	CODEC_RESP,
	CODEC_ERR,
	CODEC_NEED_DATA,
	CODEC_MAX,
	CODEC_TRANS,
	CODEC_CONN
}CodecType;

typedef enum{
	FUNC_USSD_BEGIN = 0x20,
	FUNC_USSD_END = 0x21,
	FUNC_USSD_CONTINUE = 0x22,
	FUNC_USSD_ERROR = 0x23,
	FUNC_USSD_ABORT =0x24,
	
	FUNC_MT_READ = 0x50,  //MT日志插入
	FUNC_APMINFO = 0x51,  //取应用处理模块信息
	FUNC_MO_INSERT = 0x53,  //MO日志插入
	FUNC_MT_INSERT = 0x54,  //MT日志插入
	FUNC_MT_UPDATE_ID = 0x55,  //修改MT日志唯一号
	FUNC_MT_UPDATE_RESP = 0x56,  //修改日志的回应
	
	FUNC_HEXINFO = 0xE0,//提示
	FUNC_STRINFO = 0xE1,
	FUNC_OMSENABLED = 0xE2,
	FUNC_OMSDISABLED = 0xE3,
	FUNC_CLIENTINFO = 0xE4,
	FUNC_SMS = 0xE5,
	FUNC_IF_TEST = 0xE6,
	FUNC_DB_BACKUP = 0xE7,
	FUNC_RESTART = 0xE8,
	
	// 0xF0~0xFF 为系统所用
	FUNC_ERR = 0xF1,		// 错误
	
}FuncCode;

//通信常量定义
//通信常量定义
typedef enum{
	COMM_CONNECT 	= 1,	//连接
	COMM_TERMINATE 	= 2,	//终止
	COMM_ACTIVE_TEST = 3,	//测试
	COMM_SUBMIT 	= 4,	//提交
	COMM_DELIVER 	= 5,	//转发
	COMM_ERR		= 9,		//错误报告
	COMM_RSP = 0x80000000, 	//回应标记
	COMM_MASK = 0x7FFFFFFF,	//命令掩码
	COMM_MAXSEQ = 0x0FFFFFFFF, //最大通信序列号
	COMM_ACTIVE_TEST_START = 3, //测试包测试开始次数
	COMM_ACTIVE_TEST_END = 6, //测试包测试开始次数
	COMM_RECV_MAXBUF = 1024*6,	//接收缓存的最大数
	COMM_SEND_MAXNUM = 3,		//信息发送的最大重复次数
	COMM_RECV_MAXNUM = 6,
	COMM_PENDING_TIME = 200,
	COMM_TCPSRV_MAXClIENT = 256//TCP服务器的最大客户连接数
}CommCode; 

//系统(命令)状态
typedef enum{
	STA_SUCCESS = 0,		//成功
	STA_TIMEOUT=1,			//超时
	STA_BUSY=2,				//系统忙
	STA_IF_DOWN=3,			//接口不通
	STA_MEM_ALLOC_FAILED=4,	//内存分配失败
	STA_SEND_FAILED=5,		//数据接收失败
	STA_SYSTEM_ERR=6,	    //系统错误
	STA_QOSMAX = 7,         // 阀值上限				
	STA_USRPWDERR=8,        //用户密码不正确
	STA_APM_NONEXIST=9,		//APM不存在
	STA_MSGINVALID=11,      // 消息包内容无效(格式不正确)
	STA_UNKNOWN=0xFF		//未知错误
}StaCode;

typedef enum{
	ERR_CRITICAL = 0,	//严重
	ERR_MAJOR = 1,		// 主要
	ERR_MINOR =2 ,  
	ERR_GENERAL =3,		//一般
	ERR_PROMPT = 4		//提示
}ErrClass;

typedef enum{
	ERR_ENV = 0,	
	ERR_DEV = 1,	
	ERR_COM =2 ,  
	ERR_SOFT =3,	
	ERR_QOS = 4,	
	ERR_INFO = 5	
}ErrType;

typedef struct{
        char szAppId[21];
}AppInfo;//134

//告警信息结构
typedef struct{
	BYTE btClass;
	BYTE btErrNo;
	char szModName[30];
	char szErrInfo[255];
}Alarm;

typedef struct{
	BYTE btDCS;		//2004-05-08 15:16 Yan Add
	BYTE btResp_Id[64]; 
	BYTE btStatus;	
}RespMsg;


//消息头定义
typedef struct MsgHdr{
	TKU32 unMsgLen;		//长度
	TKU32 unMsgCode;	//消息码
	TKU32 unStatus;		//消息执行状态
	TKU32 unMsgSeq;		//消息序列号	
	TKU32 unRspTnlId; 	//返回消息使用的通道号
}MsgHdr;

//消息体
typedef struct Msg{	
	char szApmId[20];
	union {
		Alarm stAlarm;
	};
}Msg;

const unsigned int MSGSTART = 22;
//系统的模块编号定义
const unsigned int MSGHDRLEN = sizeof(MsgHdr);
//const unsigned int APPINFOLEN = sizeof(AppInfo);
const unsigned int MSGPRELEN = 20;
const unsigned int MSGLEN = sizeof(Msg);

#define MAXMSGSIZE 2*1024   //(MSGHDRLEN + MSGLEN) //内存信息块大小
#define MAXMSGSIZE_L 512		//小内存信息块大小

#define MEM_MSG_CNT 512 + 50 //内存信息块数

#define QUEUE_ELEMENT_CNT 512  //队列中元素个数

//消息队列定义
typedef struct  {
	TKU32 unDirect;	//消息队列标识
	TKU32 unQLen;	//消息队列长度
}MsgQCfg;

typedef bool (*OnLogin)(void *pSock);
typedef int (*OnEncode)(void* pMsg, void* pBuf);
typedef CodecType (*OnDecode)(void* pMsg, void* pBuf, void* pRespBuf, int &RespLen, int& nUsed);

typedef struct{
	char szIniFileName[128];	//配置文件
	char szModName[30];			//模块名
    BYTE pid;               //通道号
  	BYTE self;				//模块标识
 	BYTE ctrl;				//控制模块
	BYTE remote;			//远端模块
	TKU32 unMemHdl;				//接收线程申请内存所需的句柄
	TKU32 unMemHdlSend;			//发送线程申请内存所需的句柄
 	NPType npType;				//网络通信协议  	
	bool bTcpIsSvrConn; 		//如果是客户端sock，远端地址和端口读配置文件获得
	int nTcpSocket;				//socket值
	bool bTcpIsShareMode; 		//如果是共享模式，新的消息通道由NetIfBase申请
	int nTcpDynTnlSize;			//共享模式新建的动态通道长度
	OnLogin CallbackLogin;		//Login回调函数指针，为空表示不需要Login
	OnEncode CallbackOnEncode;	//编码回调函数指针，为空表示不需要编码
	OnDecode CallbackOnDecode;	//解码回调函数指针，为空表示不需要解码
	bool bIsZeroSeq;				//测试包发送方式，"真"为收到数据时,测试发送计数清0
	bool bIsTest;				//是否进行网络测试，"真"进行
}ModIfInfo;

//共享函数定义
int HexToInt(string str);//16进制字串转整型
string IntToHex(int datalen);//整型转16进制字串
DWORD TurnDWord(const DWORD value);//4字节高低位互换
void AddMsg(unsigned char *str,int num);//显示信息内容
void AddMsg_NoSep(unsigned char *str,int num);
bool ErrMsg(TKU32 ulHandle, unsigned long ulMCBHandle, char *msg);//将消息送消息队列
int writelog(char *path,string log);//写日志文件
int UnicodeToGB(BYTE* in, char* out);
bool GetApmId(char *dest,const char *src);
//void MD5(unsigned char *szBuf, unsigned char *src, unsigned int len);
bool GetRespId(BYTE *dst, BYTE *src,int pLen);
//日志处理
bool DataLog(BYTE pSrcType, char * pTitle, char *pSrc, int pLen);
int WriteDataLog(string pLog);
string MemToStr(char *pBuf, int pLen);
bool DataLogSock(BYTE pSrcType, char *pTitle, char *pSrc, int pLen);
//信息转发
bool SendMsg(TKU32 ulHandle, unsigned long ulMCBHandle, char *msg);//将消息送消息队列
bool SendMsg_Ex(TKU32 ulHandle, unsigned long ulMCBHandle, char *msg, int sendLen);
bool SendToOms(TKU32 pHandle, int pTBLMUCBEnum, bool pEnabled, BYTE pMod,char *pApmId, BYTE pMsgType, char *pSrc, int pLen);

//日期转换
void Time2Chars(time_t* t_time, char* pTime);
void Str2time(char* strTime, time_t* ntime);

typedef CodecType (*OnDecodeReader)(unsigned char* pMsg, int& nUsed, unsigned char* out, BYTE msgType);
typedef int (*OnEncodeReader)(unsigned char id, unsigned char* pMsg, int sendlen, unsigned char* out, unsigned char msgType);
string GetNowtimeStr();
string BytesToStr(BYTE *pBuf, int pLen);
short ByteToShort(BYTE *pBuf);
void strRightFillSpace(const char *src, int inLen,  char* out, int outlen);

bool filecopy(char* srcfile, char* dstfile);

char* ltrim(char* s, int len);
char* rtrim(char* s, int len);
char* trim(char* s, int len);
TKU16 ConvertUShort(TKU16 value);
void DEBUG_LOG(char* pStr);

int  String2Hex(unsigned char* pDest, unsigned char* pSrc, int iLenSrc);
int  Hex2String(char* pSrc, char* pDest, int iLenDest);

void transPolish(char* szRdCalFunc, char* tempCalFunc);         //计算式转逆波兰式
float compvalue(char* tempCalFunc, float* midResult);           //逆波兰式算结果
#pragma pack()

#endif
