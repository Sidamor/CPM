#include <net/if.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>
#include <termio.h>
#include <stdio.h>
#include "Init.h"
#include "HardIF.h"
#include "Crc.h"

/************************************************************************/
/* 系统资源定义                                                         */
/************************************************************************/
//消息队列内存定义
static MMUserCtrlBlkStruct g_stHubMUCBArray[MUCB_MAX] = {
	/*
	{用户内存控制块句柄,	用户请求的每条信息的尺寸,		实际提供的每元素尺寸,	元素数量,			内存区尺寸}
	*/
	{0,						MAXMSGSIZE*2,					0,						(MEM_MSG_CNT), 		0},
	{1,						MAXMSGSIZE*2,					0,						(MEM_MSG_CNT), 		0},

	{2,						MAXMSGSIZE*2,					0,						(MEM_MSG_CNT),		0},
	{3,						MAXMSGSIZE*2,					0,						(MEM_MSG_CNT), 		0},

	{4,						MAXMSGSIZE*2,					0,						(MEM_MSG_CNT),		0},
	{5,						MAXMSGSIZE*2,					0,						(MEM_MSG_CNT), 		0},

	{6,						MAXMSGSIZE/2,					0,						(MEM_MSG_CNT)/2, 	0},
	{7,						MAXMSGSIZE/2,					0,						(MEM_MSG_CNT)/2, 	0},
	{8,						MAXMSGSIZE/2,					0,						(MEM_MSG_CNT)/2, 	0},
    {9,						MAXMSGSIZE/2,					0,						(MEM_MSG_CNT)/2, 	0},
	{10,					MAXMSGSIZE/2,					0,						(MEM_MSG_CNT)/2, 	0},
	{11,					MAXMSGSIZE/2,					0,						(MEM_MSG_CNT)/2, 	0},
	{12,					MAXMSGSIZE/2,					0,						(MEM_MSG_CNT)/2,	0},
	{13,					MAXMSGSIZE/2,					0,						(MEM_MSG_CNT)/2, 	0},

	{14,					MAXMSGSIZE/2,					0,						(MEM_MSG_CNT)/2, 	0},
	{15,					MAXMSGSIZE/2,					0,						(MEM_MSG_CNT)/2, 	0},
	{16,					MAXMSGSIZE/2,					0,						(MEM_MSG_CNT), 		0},

	{17,					MAXMSGSIZE/2,					0,						(MEM_MSG_CNT)*2, 	0},
	{18,					MAXMSGSIZE,						0,						(MEM_MSG_CNT),		0},
	{19,					MAXMSGSIZE,						0,						(MEM_MSG_CNT),		0},

	{20,					MAXMSGSIZE/2,					0,						(MEM_MSG_CNT), 		0},
	{21,					MAXMSGSIZE,						0,						(MEM_MSG_CNT)/2,	0},

//	{22,					MAXMSGSIZE,						0,						(MEM_MSG_CNT)/2,	0},
	{22,					MAXMSGSIZE,						0,						(MEM_MSG_CNT),		0},

	{23,					MAXMSGSIZE/2,					0,						(MEM_MSG_CNT),		0}
};
//消息队列的定义
MsgQCfg stQMC[] = 
{
	//网页配置的部分，在下面直接初始化
	{COMBINE_MOD(MOD_CTRL, MOD_ACTION_NET),				QUEUE_ELEMENT_CNT},					//网络任务队列

	{COMBINE_MOD(MOD_CTRL, MOD_DEV_ANALYSE),			QUEUE_ELEMENT_CNT*2},				//轮巡结果待分析队列

	{COMBINE_MOD(MOD_CTRL, MOD_DEV_MSGCTRL_BOA),		QUEUE_ELEMENT_CNT},					//网络指令处理结果队列
	{COMBINE_MOD(MOD_CTRL, MOD_DEV_MSGCTRL_TMN),		QUEUE_ELEMENT_CNT},					//网络指令处理结果队列

	{COMBINE_MOD(MOD_CTRL, MOD_DEV_SMS_RECV),			QUEUE_ELEMENT_CNT},					//短消息接收队列
	{COMBINE_MOD(MOD_CTRL, MOD_DEV_SMS_SEND),			QUEUE_ELEMENT_CNT/2},				//短消息发送队列
	//
	{COMBINE_MOD(MOD_CTRL, MOD_DEV_NET_ACTION),			QUEUE_ELEMENT_CNT},					//网络动作执行队列
	{COMBINE_MOD(MOD_CTRL, MOD_DEV_TRANSPARENT),		QUEUE_ELEMENT_CNT}					//透明传输消息处理
	
};
//内存管理模块
CMemMgr			g_MemMgr; 							//内存块管理模块
CMsgMgrBase		g_MsgMgr;

CDevInfoContainer g_DevInfoContainer;

/************************************************************************/
/* 网络通信模块-TCP Server                                              */
/************************************************************************/
//TCP Server模块
ModIfInfo stIfTcpSvrBoa = {
	"Config.ini",
	"MOD_IF_SVR_BOA",
	1,
	MOD_IF_SVR_NET_BOA,
	MOD_IF_SVR_CTRL_BOA,
	MOD_IF_SVR_NET_BOA,
	MUCB_IF_NET_SVR_RCV_BOA,
	MUCB_IF_NET_SVR_SEND_BOA,
	NP_TCPSVR,
	true,
	-1,
	true,
	1024,
	NULL,    		// OnLogin CallbackLogin;
	NULL,     		// OnEncode CallbackOnEncode;
	DeCodeSVR,      // OnDecode CallbackOnDecode;
	true,
	true
};
ModIfInfo stIfTcpSvrTmn = {
	"Config.ini",
	"MOD_IF_SVR_TMN",
	2,
	MOD_IF_SVR_NET_TMN,
	MOD_IF_SVR_CTRL_TMN,
	MOD_IF_SVR_NET_TMN,
	MUCB_IF_NET_SVR_RCV_TMN,
	MUCB_IF_NET_SVR_SEND_TMN,
	NP_TCPSVR,
	true,
	-1,
	true,
	1024,
	NULL,    		// OnLogin CallbackLogin;
	NULL,     		// OnEncode CallbackOnEncode;
	DeCodeSVR,      // OnDecode CallbackOnDecode;
	true,
	true
};
//TCP Server管理模块
CRouteMgr			g_RouteMgr;
ConnContainer		g_ConnContainer;
//BOA
ClsSvrConnMgr		ConnMgrHubBoa(COMM_TCPSRV_MAXClIENT);		//Boa,Count=1
CNetIfBase			g_NetIfSvrBoa(stIfTcpSvrBoa);				//NetIf的容器
CBOAMsgCtlSvr		g_MsgCtlSvrBoa;								//TCP Server网络数据处理模块
TKU32				g_ulHandleNetBoaSvrSend;					//网络发送队列句柄
TKU32				g_ulHandleNetBoaSvrRecv;					//网络接收队列句柄
TKU32				g_ulHandleNetSvrBoaAction;					//网络动作执行队列句柄
//TERMINAL
ClsSvrConnMgr		ConnMgrHubTmn(COMM_TCPSRV_MAXClIENT);
CNetIfBase			g_NetIfSvrTmn(stIfTcpSvrTmn);				//NetIf的容器
CTMNMsgCtlSvr		g_MsgCtlSvrTmn;								//TCP Server网络数据处理模块
TKU32				g_ulHandleNetTmnSvrSend;					//网络发送队列句柄
TKU32				g_ulHandleNetTmnSvrRecv;					//网络接收队列句柄
TKU32				g_ulHandleNetSvrTmnAction;					//网络动作执行队列句柄

/************************************************************************/
/* 网络通信模块-TCP Client  连接平台                                    */
/************************************************************************/
ModIfInfo stIfNet = {
	"Config.ini",
	"PARENT",
    0,
	MOD_IF_CLI_NET,
	MOD_IF_CLI_CTRL,
	MOD_IF_CLI_NET,
	MUCB_IF_NET_CLI_RCV,
	MUCB_IF_NET_CLI_SEND,
	NP_TCP,
	false,
	-1,
	false,
	-1,
	LoginSvr,
	NULL,
	NULL,
	true,
	true	
};
//客户端
char			g_szEdId[24];
char			g_szPwd[20];
int				g_upFlag = 0;						//连接平台标记，0不连接，1连接

CNetIfBase		g_NetIfTcpClient(stIfNet);          //Client网络连接容器
CMsgCtlClient	g_MsgCtlCliNet;						//Client网络数据处理模块
TKU32			g_ulHandleNetCliSend;				//网络发送队列句柄
TKU32			g_ulHandleNetCliRecv;				//网络接收队列句柄

CNetDataContainer	g_NetContainer;							//网络接口数据容器，用于接收通过网络传输的传感数据

/************************************************************************/
/* CPM接口模块                                                          */
/************************************************************************/
/*系统口*/
//CADCBase	g_CADCBaseA(1);/*/dev/spidev1.1口*/
//CADCBase	g_CADCBaseB(0);/*/dev/spidev1.2口*/

//int			g_hdlGPIO = -1;/*GPIO接口句柄*/

/*GSM模块*/
//CGSMBaseSiemens		g_GSMBaseSiemens;
CGSMCtrl			g_GSMCtrl;						//GSM短信消息处理模块
TKU32				g_ulHandleGSM;					//GSM控制队列句柄
TKU32				g_ulHandleSMSSend;				//短消息发送队列句柄
TKU32				g_ulHandleSMSRecv;				//短消息接收队列句柄

//485主控模块（轮巡，执行动作）
CDeviceIfBase* g_ptrDeviceIf485_1;
CDeviceIfBase* g_ptrDeviceIf485_2;
CDeviceIfBase* g_ptrDeviceIf485_3;
CDeviceIfBase* g_ptrDeviceIf485_4;
CDeviceIfBase* g_ptrDeviceIf485_5;
CDeviceIfBase* g_ptrDeviceIf485_6;
CDeviceIfBase* g_ptrDeviceIf485_7;
CDeviceIfBase* g_ptrDeviceIf485_8;

TKU32 g_ulHandleActionList481_1;					//
TKU32 g_ulHandleActionList481_2;					//
TKU32 g_ulHandleActionList481_3;					//
TKU32 g_ulHandleActionList481_4;					//
TKU32 g_ulHandleActionList481_5;					//
TKU32 g_ulHandleActionList481_6;					//
TKU32 g_ulHandleActionList481_7;					//
TKU32 g_ulHandleActionList481_8;					//

//开关量、模拟量
CDeviceIfBase*	g_ptrDeviceIfAD;
TKU32 g_ulHandleActionListAD;					//

//数据分析模块
CAnalyser g_Analyser;								//数据分析模块
TKU32 g_ulHandleAnalyser;							//数据分析模块待解析队列句柄


/************************************************************************/
/* sqlite3数据库处理模块                                                */
/************************************************************************/
CSqlCtl g_SqlCtl;

/************************************************************************/
/* 其他                                                                 */
/************************************************************************/
TKU32			g_ulHandleBoaMsgCtrl;					//异步任务回应队列句柄
TKU32			g_ulHandleTmnMsgCtrl;					//异步任务回应队列句柄

/************************************************************************/
/* 声卡，音频                                                           */
/************************************************************************/
//CAudioPlay g_AudioPlay;

/************************************************************************/
/* 读卡模块	                                                            */
/************************************************************************/

char				g_szLocalIp[24] = {0};							//本机ip
char				g_szVersion[20] = {0};							//CPM版本号
int					g_iTTLSampleCount = 1;						//开关量采样次数
int					g_iADSampleCount = 30;						//模拟量采样次数

/************************************************************************/
/* 模块配置	                                                            */
/************************************************************************/
MODE_CFG g_ModeCfg;

/************************************************************************/
/* 记录条数配置	                                                        */
/************************************************************************/
RECORD_COUNT_CFG g_RecordCntCfg;

/************************************************************************/
/* 定时任务配置	                                                        */
/************************************************************************/
COnTimeCtrl g_COnTimeCtrl;


/************************************************************************/
/* 透明传输业务模块	                                                        */
/************************************************************************/
CTransparent g_CTransparent;
TKU32 g_ulHandleTransparent;

bool CInit::Initialize(void)
{
//读取Config.ini
	ReadConfigFile();
	
//sqlite3数据库
	if (false == g_SqlCtl.Initialize(DB_TODAY_PATH))return false;

	//获取接口配置信息
	memset((unsigned char*)&g_ModeCfg, 0, sizeof(MODE_CFG));
	g_SqlCtl.getModeConfig(g_ModeCfg);

	//获取系统配置条数信息
	memset((unsigned char*)&g_RecordCntCfg, 0, sizeof(RECORD_COUNT_CFG));
	g_SqlCtl.getRecordCountConfig(g_RecordCntCfg);
	
//内存初始化
	if(false == MemInit()) return false;			//存储初始化

//设备初始化
	if(false == g_DevInfoContainer.Initialize())return false;

//硬件设备初始化
	//GSM底层发送、接收队列
	g_ulHandleSMSSend = 0;			
	if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_CTRL, MOD_DEV_SMS_SEND), g_ulHandleSMSSend) )//短消息发送队列句柄
		return false;
	DBG(("Init g_ulHandleSMSSend[%d]\n", g_ulHandleSMSSend));
	g_ulHandleSMSRecv = 0;			
	if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_CTRL, MOD_DEV_SMS_RECV), g_ulHandleSMSRecv) )//短消息接收队列句柄
		return false;
	DBG(("Init g_ulHandleSMSRecv[%d]\n", g_ulHandleSMSRecv));

	STR_SMS_CFG strSmsCfg;
	strSmsCfg.ulHandleGSMSend = g_ulHandleSMSSend;
	strSmsCfg.ulHandleGSMRecv = g_ulHandleSMSRecv;
	HardIF_Initialize(CDevInfoContainer::ADCValueNoticeCallBack, (void*)&g_DevInfoContainer, strSmsCfg);

	//485_1
	if(1 == g_ModeCfg.RS485_1)
	{
		g_ulHandleActionList481_1 = 0;			
		if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_CTRL, MOD_ACTION_485_1), g_ulHandleActionList481_1) )//获取485-1动作队列句柄
			return false;
		g_ptrDeviceIf485_1 = new CDeviceIf485();
		if (SYS_STATUS_SUCCESS != g_ptrDeviceIf485_1->Initialize(INTERFACE_RS485_1, g_ulHandleActionList481_1))return false;
	}

	//485_2
	if(1 == g_ModeCfg.RS485_2)
	{
		g_ulHandleActionList481_2 = 0;			
		if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_CTRL, MOD_ACTION_485_2), g_ulHandleActionList481_2) )//获取485-2动作队列句柄
			return false;
		g_ptrDeviceIf485_2 = new CDeviceIf485();
		if (SYS_STATUS_SUCCESS != g_ptrDeviceIf485_2->Initialize(INTERFACE_RS485_2, g_ulHandleActionList481_2))return false;
	}

	//485_3
	if(1 == g_ModeCfg.RS485_3)
	{
		g_ulHandleActionList481_3 = 0;			
		if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_CTRL, MOD_ACTION_485_3), g_ulHandleActionList481_3) )//获取485-3动作队列句柄
			return false;
		g_ptrDeviceIf485_3 = new CDeviceIf485();
		if (SYS_STATUS_SUCCESS != g_ptrDeviceIf485_3->Initialize(INTERFACE_RS485_3, g_ulHandleActionList481_3))return false;
	}

	//485_4
	if(1 == g_ModeCfg.RS485_4)
	{
		g_ulHandleActionList481_4 = 0;			
		if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_CTRL, MOD_ACTION_485_4), g_ulHandleActionList481_4) )//获取485-4动作队列句柄
			return false;
		g_ptrDeviceIf485_4 = new CDeviceIf485();
		if (SYS_STATUS_SUCCESS != g_ptrDeviceIf485_4->Initialize(INTERFACE_RS485_4, g_ulHandleActionList481_4))return false;
	}

	//485_5
	if(1 == g_ModeCfg.RS485_5)
	{
		g_ulHandleActionList481_5 = 0;			
		if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_CTRL, MOD_ACTION_485_5), g_ulHandleActionList481_5) )//获取485-5动作队列句柄
			return false;
		g_ptrDeviceIf485_5 = new CDeviceIf485();	
		if (SYS_STATUS_SUCCESS != g_ptrDeviceIf485_5->Initialize(INTERFACE_RS485_5, g_ulHandleActionList481_5))return false;
	}

	//485_6
	if(1 == g_ModeCfg.RS485_6)
	{
		g_ulHandleActionList481_6 = 0;			
		if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_CTRL, MOD_ACTION_485_6), g_ulHandleActionList481_6) )//获取485-6动作队列句柄
			return false;
		g_ptrDeviceIf485_6 = new CDeviceIf485();
		if (SYS_STATUS_SUCCESS != g_ptrDeviceIf485_6->Initialize(INTERFACE_RS485_6, g_ulHandleActionList481_6))return false;
	}

	//485_7
	if(1 == g_ModeCfg.RS485_7)
	{
		g_ulHandleActionList481_7 = 0;			
		if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_CTRL, MOD_ACTION_485_7), g_ulHandleActionList481_7) )//获取485-7动作队列句柄
			return false;
		g_ptrDeviceIf485_7 = new CDeviceIf485();
		if (SYS_STATUS_SUCCESS != g_ptrDeviceIf485_7->Initialize(INTERFACE_RS485_7, g_ulHandleActionList481_7))return false;
	}

	//485_8
	if(1 == g_ModeCfg.RS485_8)
	{
		g_ulHandleActionList481_8 = 0;			
		if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_CTRL, MOD_ACTION_485_8), g_ulHandleActionList481_8) )//获取485-8动作队列句柄
			return false;
		g_ptrDeviceIf485_8 = new CDeviceIf485();
		if (SYS_STATUS_SUCCESS != g_ptrDeviceIf485_8->Initialize(INTERFACE_RS485_8, g_ulHandleActionList481_8))return false;
	}

	//AD
	if(1 == g_ModeCfg.Ad)
	{
		g_ulHandleActionListAD = 0;			
		if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_CTRL, MOD_ACTION_ADS), g_ulHandleActionListAD) )//获取模拟量开关量动作队列句柄
			return false;
		g_ptrDeviceIfAD = new CDeviceIfAD();
		if (SYS_STATUS_SUCCESS != g_ptrDeviceIfAD->Initialize(INTERFACE_AD_A0, g_ulHandleActionListAD))return false;
	}

	if(false == TcpClientInit()) return false;

	if(false == TcpServerInit()) return false;

	//初始化网络传感接口
	if(1 == g_ModeCfg.NetTerminal)
	{
		if( false == g_NetContainer.Initialize()) return false;
	}

	//GSM轮询、分析队列
	g_ulHandleGSM = 0;			
	if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_CTRL, MOD_ACTION_GSM), g_ulHandleGSM) )//短消息发送队列句柄
		return false;
	if(1 == g_ModeCfg.GSM)
	{
		if(!g_GSMCtrl.Initialize(g_ulHandleGSM))
			return false;
	}

	//透明传输业务处理模块
	if(1 == g_ModeCfg.Transparent)
	{
		g_ulHandleTransparent = 0;			
		if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_CTRL, MOD_DEV_TRANSPARENT), g_ulHandleTransparent) )//获取中海油数据处理队列句柄
			return false;
		g_CTransparent.Initialize(g_ulHandleTransparent);
	}

	//网络网页消息处理模块
	g_ulHandleBoaMsgCtrl = 0;
	if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_CTRL, MOD_DEV_MSGCTRL_BOA), g_ulHandleBoaMsgCtrl) )		//获取网络指令处理结果队列句柄
		return false;
	if(!g_MsgCtlSvrBoa.Initialize(COMBINE_MOD(stIfTcpSvrBoa.self,stIfTcpSvrBoa.ctrl), "TCPSVR_BOA处理线程启动"))
	{
		DBG(("BoaMsgCtlSvr Initialize failed!\n"));
		return false;
	}

	//网络终端消息处理模块
	g_ulHandleTmnMsgCtrl = 0;
	if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_CTRL, MOD_DEV_MSGCTRL_TMN), g_ulHandleTmnMsgCtrl) )		//获取网络指令处理结果队列句柄
		return false;
	if(!g_MsgCtlSvrTmn.Initialize(COMBINE_MOD(stIfTcpSvrTmn.self,stIfTcpSvrTmn.ctrl),"TCPSVR_TMN处理线程启动"))
	{
		DBG(("MsgCtlSvr Initialize failed!\n"));
		return false;
	}

	//分析模块
	g_ulHandleAnalyser = 0;			
	if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_CTRL, MOD_DEV_ANALYSE), g_ulHandleAnalyser) )//获取数据分析模块待解析队列句柄
		return false;

	if(false == g_Analyser.Initialize(g_ulHandleAnalyser))return false;         //启动数据分析模块

	//定时任务启动
	if(false == g_COnTimeCtrl.Initialize())return false;

	for (int i=0; i<3; i++)
	{
		HardIF_AllLightON();
		sleep(1);
		HardIF_AllLightOFF();
		sleep(1);
	}

	DBG(("CInit::Initialize success\n"));
	return true;
}
void CInit::Terminate(void)
{

}

//申请消息通道的索引空间，系统消息通道MUCB_MAX数
bool CInit::MemInit()
{
	if (MMO_OK != g_MemMgr.Initialize(MUCB_MAX))
	{
		DBG(("MMInitialize failed!\n"));
		return false;
	}	
	for (int i = 0; i < MUCB_MAX; i++)// 给消息索引赋值，并给申请相应的空间
	{
		if (!g_MemMgr.RegRsvMem(&g_stHubMUCBArray[i]))
		{
			return false;
		}
	}
	if (! g_MsgMgr.Initialize(SYS_MOD_MAX))//消息队列申请头数组
	{
		DBG(("MsgMgr.Initialize failed\n"));
		return false;
	}

	//申请消息通道，并进行消息队列初始化 lsd网页配置部分
	size_t nTID = 0;
	//客户端网络接收队列
	if (MMR_OK != g_MsgMgr.AllocTunnel(COMBINE_MOD(MOD_IF_CLI_NET, MOD_IF_CLI_CTRL), g_RecordCntCfg.NetClientRecv_Max, nTID))
	{
		DBG(("SetTunnel failed\n"));
		return false;
	}
	//客户端网络发送队列	
	if (MMR_OK != g_MsgMgr.AllocTunnel(COMBINE_MOD(MOD_IF_CLI_CTRL, MOD_IF_CLI_NET), g_RecordCntCfg.NetClientSend_Max, nTID))
	{
		DBG(("SetTunnel failed\n"));
		return false;
	} 

	//网页服务接收队列
	if (MMR_OK != g_MsgMgr.AllocTunnel(COMBINE_MOD(MOD_IF_SVR_NET_BOA, MOD_IF_SVR_CTRL_BOA), g_RecordCntCfg.NetRecv_Max, nTID))
	{
		DBG(("SetTunnel failed\n"));
		return false;
	}
	//网页服务发送队列	
	if (MMR_OK != g_MsgMgr.AllocTunnel(COMBINE_MOD(MOD_IF_SVR_CTRL_BOA, MOD_IF_SVR_NET_BOA), g_RecordCntCfg.NetSend_Max, nTID))
	{
		DBG(("SetTunnel failed\n"));
		return false;
	} 

	//终端服务接收队列
	if (MMR_OK != g_MsgMgr.AllocTunnel(COMBINE_MOD(MOD_IF_SVR_NET_TMN, MOD_IF_SVR_CTRL_TMN), g_RecordCntCfg.NetRecv_Max, nTID))
	{
		DBG(("SetTunnel failed\n"));
		return false;
	}
	//终端服务发送队列	
	if (MMR_OK != g_MsgMgr.AllocTunnel(COMBINE_MOD(MOD_IF_SVR_CTRL_TMN, MOD_IF_SVR_NET_TMN), g_RecordCntCfg.NetSend_Max, nTID))
	{
		DBG(("SetTunnel failed\n"));
		return false;
	} 


	//485-1任务队列
	if (MMR_OK != g_MsgMgr.AllocTunnel(COMBINE_MOD(MOD_CTRL, MOD_ACTION_485_1), g_RecordCntCfg.RS485_1_Max, nTID))
	{
		DBG(("SetTunnel failed\n"));
		return false;
	}
	//485-2任务队列
	if (MMR_OK != g_MsgMgr.AllocTunnel(COMBINE_MOD(MOD_CTRL, MOD_ACTION_485_2), g_RecordCntCfg.RS485_2_Max, nTID))
	{
		DBG(("SetTunnel failed\n"));
		return false;
	}
	//485-3任务队列
	if (MMR_OK != g_MsgMgr.AllocTunnel(COMBINE_MOD(MOD_CTRL, MOD_ACTION_485_3), g_RecordCntCfg.RS485_3_Max, nTID))
	{
		DBG(("SetTunnel failed\n"));
		return false;
	}
	//485-4任务队列
	if (MMR_OK != g_MsgMgr.AllocTunnel(COMBINE_MOD(MOD_CTRL, MOD_ACTION_485_4), g_RecordCntCfg.RS485_4_Max, nTID))
	{
		DBG(("SetTunnel failed\n"));
		return false;
	}
	//485-5任务队列
	if (MMR_OK != g_MsgMgr.AllocTunnel(COMBINE_MOD(MOD_CTRL, MOD_ACTION_485_5), g_RecordCntCfg.RS485_5_Max, nTID))
	{
		DBG(("SetTunnel failed\n"));
		return false;
	}
	//485-6任务队列
	if (MMR_OK != g_MsgMgr.AllocTunnel(COMBINE_MOD(MOD_CTRL, MOD_ACTION_485_6), g_RecordCntCfg.RS485_6_Max, nTID))
	{
		DBG(("SetTunnel failed\n"));
		return false;
	}
	//485-7任务队列
	if (MMR_OK != g_MsgMgr.AllocTunnel(COMBINE_MOD(MOD_CTRL, MOD_ACTION_485_7), g_RecordCntCfg.RS485_7_Max, nTID))
	{
		DBG(("SetTunnel failed\n"));
		return false;
	}
	//485-8任务队列
	if (MMR_OK != g_MsgMgr.AllocTunnel(COMBINE_MOD(MOD_CTRL, MOD_ACTION_485_8), g_RecordCntCfg.RS485_8_Max, nTID))
	{
		DBG(("SetTunnel failed\n"));
		return false;
	}
	//ADS任务队列
	if (MMR_OK != g_MsgMgr.AllocTunnel(COMBINE_MOD(MOD_CTRL, MOD_ACTION_ADS), g_RecordCntCfg.ADS_Max, nTID))
	{
		DBG(("SetTunnel failed\n"));
		return false;
	}
	//GSM任务队列
	if (MMR_OK != g_MsgMgr.AllocTunnel(COMBINE_MOD(MOD_CTRL, MOD_ACTION_GSM), g_RecordCntCfg.GSM_Max, nTID))
	{
		DBG(("SetTunnel failed\n"));
		return false;
	}
	//默认部分lsd
	for (int i = 0; i < (int)(sizeof(stQMC) / sizeof(MsgQCfg)); i++)//申请消息通道，并进行消息队列初始化
	{
		if (MMR_OK != g_MsgMgr.AllocTunnel(stQMC[i].unDirect, stQMC[i].unQLen, nTID))
		{
			DBG(("SetTunnel failed\n"));
			return false;
		} 
	}
	return true;
}


//启动TcpServer服务
bool CInit::TcpServerInit()
{
	//BOA
	g_ulHandleNetBoaSvrRecv = 0;
	if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_IF_SVR_NET_BOA, MOD_IF_SVR_CTRL_BOA), g_ulHandleNetBoaSvrRecv) )//获取TcpServer网关接收队列句柄
		return false;

	g_ulHandleNetBoaSvrSend = 0;	
	if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_IF_SVR_CTRL_BOA, MOD_IF_SVR_NET_BOA), g_ulHandleNetBoaSvrSend) )//获取TcpServer网关发送队列句柄
		return false;

	if(false == g_NetIfSvrBoa.Initialize())           // TCPServer
	{
		DBG(("g_NetIfSvrBoa.Initialize failed\n"));
		return false;
	}

	//TERMINAL
	g_ulHandleNetTmnSvrRecv = 0;
	if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_IF_SVR_NET_TMN, MOD_IF_SVR_CTRL_TMN), g_ulHandleNetTmnSvrRecv) )//获取TcpServer网关接收队列句柄
		return false;

	g_ulHandleNetTmnSvrSend = 0;	
	if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_IF_SVR_CTRL_TMN, MOD_IF_SVR_NET_TMN), g_ulHandleNetTmnSvrSend) )//获取TcpServer网关发送队列句柄
		return false;

	if(false == g_NetIfSvrTmn.Initialize())           // TCPServer
	{
		DBG(("g_NetIfSvrTmn.Initialize failed\n"));
		return false;
	}

	//给远程接入的TCP客户端申请堆栈和哈希表
	if (false == g_ConnContainer.Initialize(COMM_TCPSRV_MAXClIENT))
	{
		return false;
	}	

	//给远程接入的TCP客户端申请接收通道ID和发送通道的ID哈希表，A2I，I2A
	if (false == g_RouteMgr.Initialize(COMM_TCPSRV_MAXClIENT))
	{
		return false;
	}

	if (false == TcpSvrBoa())
	{
		return false;
	}

	if (false == TcpSvrTmn())
	{
		return false;
	}

	DBG(("TcpServer init success!\n"));
	return true;
}

//启动Tcp Client
bool CInit::TcpClientInit()
{
	g_ulHandleNetCliRecv = 0;
	if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_IF_CLI_NET, MOD_IF_CLI_CTRL), g_ulHandleNetCliRecv) )//获取上级网关接收队列句柄
		return false;

	g_ulHandleNetCliSend = 0;	
	if(MMR_OK != g_MsgMgr.Attach( COMBINE_MOD(MOD_IF_CLI_CTRL, MOD_IF_CLI_NET), g_ulHandleNetCliSend) )//获取上级网关发送队列句柄
		return false;

	if( g_upFlag == 1 )
	{
		if(false == g_NetIfTcpClient.Initialize())		// TCPClient
		{
			DBG(("g_NetIfTcpClient.Initialize failed\n"));
			return true;
		}
		if(!g_MsgCtlCliNet.Initialize(COMBINE_MOD(stIfNet.self,stIfNet.ctrl),"TcpClient处理线程启动"))
		{
			DBG(("szMsgCtlBcmpp Initialize failed!\n"));
			return true;
		}
	}
	
	DBG(("TcpClientInit success\n"));
	return true;
}

bool CInit::TcpSvrBoa()
{
	//创建TCP server	
	ConnMgrHubBoa.SetModIfInfo(stIfTcpSvrBoa, &g_NetIfSvrBoa);

	int port;
	char ip[20] = {0};

	//if (-1 == INI_READ_STRING(stIfTcpSvr, "LocalAddr", ip)  ||           //取得IP和端口号
	int sock;
	struct sockaddr_in sin;
	struct ifreq ifr;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
	{
		return false;
	}

	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;

	if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
	{
		return false;
	}

	memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
	sprintf(ip, "%s", inet_ntoa(sin.sin_addr));
	DBG(("ip:[%s]\n", ip));

	if( -1 == INI_READ_INT(stIfTcpSvrBoa, "LocalPort", port) )
		return false;

	DBG(("Net If Init....ip[%s] port[%d]\n", ip, port));
	ClsInetAddress addr(ip);
	if (false == ConnMgrHubBoa.ListenAt(addr, port))                        //监听IP和端口
	{
		DBG(("Fail at :  connMgr.ListenAt('%s', %d)\n", ip, port));
		return false;
	}
	strcpy(g_szLocalIp, ip);
	DBG(("G_LocalIp[%s]\n", g_szLocalIp));
	//char buf[256] = {0};
	//ConnMgrHub.GetSvrLocal(buf, sizeof(buf));
	return true; 
}

bool CInit::TcpSvrTmn()
{
	//创建TCP server	
	ConnMgrHubTmn.SetModIfInfo(stIfTcpSvrTmn, &g_NetIfSvrTmn);

	int port;
	char ip[20] = {0};

	//if (-1 == INI_READ_STRING(stIfTcpSvr, "LocalAddr", ip)  ||           //取得IP和端口号
	int sock;
	struct sockaddr_in sin;
	struct ifreq ifr;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
	{
		return false;
	}

	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;

	if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
	{
		return false;
	}

	memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
	sprintf(ip, "%s", inet_ntoa(sin.sin_addr));
	DBG(("ip:[%s]\n", ip));

	if( -1 == INI_READ_INT(stIfTcpSvrTmn, "LocalPort", port) )
		return false;

	DBG(("Net If Init....ip[%s] port[%d]\n", ip, port));
	ClsInetAddress addr(ip);
	if (false == ConnMgrHubTmn.ListenAt(addr, port))                        //监听IP和端口
	{
		DBG(("Fail at :  connMgr.ListenAt('%s', %d)\n", ip, port));
		return false;
	}
	strcpy(g_szLocalIp, ip);
	DBG(("G_LocalIp[%s]\n", g_szLocalIp));
	//char buf[256] = {0};
	//ConnMgrHub.GetSvrLocal(buf, sizeof(buf));
	return true; 
}

bool CInit::WaitForCmd()
{
	//为测试用
	char Cmd[256] = {0};
	bool test = true;
	int iTime = -1;
	int iWatchDogHandle = -1;
	int iDogFlag = 0;
	int iResetFlag = 0;
	get_ini_int("Config.ini","SYSTEM","WatchDog", 0, &iDogFlag);
	get_ini_int("Config.ini","SYSTEM","ResetEnable", 0, &iResetFlag);
	bool bDogOpen = iDogFlag == 1?true:false;
	DBG(("DogFlag[%d] iResetFlag[%d]\n",iDogFlag, iResetFlag));
	if( bDogOpen )
	{
		iWatchDogHandle = open ("/dev/watchdog", O_RDWR);//O_RDWR
		if (0 > iWatchDogHandle)
		{
			DBG(("开门狗设备打开失败，请确认是否是最新内核\n"));
			return false;
		}
		//开启看门狗 
		ioctl(iWatchDogHandle, WDIOC_SETOPTIONS, WDIOS_ENABLECARD);

		iTime = 20;
		printf("Set watchdog timer %d seconds\n", iTime);
		ioctl(iWatchDogHandle, WDIOC_SETTIMEOUT, &iTime);

		ioctl(iWatchDogHandle, WDIOC_GETTIMEOUT, &iTime);
		printf("Time = %d\n", iTime);
	}

	DBG(("CInit::WaitForCmd .... Pid[%d]\n", getpid()));
	bool g_bThrdReadStop = false;
	int resetFlag = 0;
	while (!g_bThrdReadStop)
	{	
		if (bDogOpen)
		{
			write(iWatchDogHandle, "1", 1);//喂狗
		}

		g_COnTimeCtrl.OnTimeCtrlProc();
		//DBG(("GDO0 [%d]\n", EmApiGetGDO0(g_hdlGPIO)));

		//复位判断
		if(1 == iResetFlag)
		{
			if(0 == HardIF_GetGD0())
				resetFlag++;
			else
				resetFlag = 0;
			if(5 == resetFlag)
			{				
				if(-1 == system("cp -r /root/network_bak /root/network"))
				{
					printf("cp -r /root/network_bak /root/network failed \n");
					resetFlag = 0;
					continue;
				}
				if (-1 == system("chmod +x /root/network"))
				{
					printf("chmod +x /root/network failed \n");
					resetFlag = 0;
					continue;
				}
				if (-1 == system("/root/network"))
				{
					printf("/root/network failed \n");
					resetFlag = 0;
					continue;
				}
				printf("end reset network resetFlag[%d]\n",resetFlag);
			}
		}
		if( test )
		{
			sleep(1);
			continue;
		}
		memset(Cmd, 0, sizeof(Cmd));
		scanf("%s", Cmd);
		switch (atoi(Cmd))
		{
		case 0:
			g_bThrdReadStop = true;
			break;
		case 1://黑名单加1
			{

			}
			break;
		case 3://RF433发送测试
			{

			}
			break;				
		}
		sleep(1);
	}
	return true;
}


void CInit::ReadConfigFile()
{
	memset(g_szEdId, 0, sizeof(g_szEdId));
	memset(g_szPwd, 0, sizeof(g_szPwd));
	g_upFlag = 0;
	memset(g_szVersion, 0, sizeof(g_szVersion));

	get_ini_string("Config.ini","PARENT","RemoteLoginId" ,"=", g_szEdId, sizeof(g_szEdId));
	get_ini_string("Config.ini","PARENT","RemoteLoginPwd" ,"=", g_szPwd, sizeof(g_szPwd));
    get_ini_int("Config.ini","PARENT","RemoteStatus", 0, &g_upFlag);

	get_ini_string("Config.ini","SYSTEM","Version" ,"=", g_szVersion, sizeof(g_szVersion));

    get_ini_int("Config.ini","INTER_CONFIG","TTLSampleCount", 0, &g_iTTLSampleCount);
    get_ini_int("Config.ini","INTER_CONFIG","ADSampleCount", 0, &g_iADSampleCount);

	DBG(("RemoteLoginId[%s] RemoteLoginPwd[%s] RemoteStatus[%d] Version[%s] TTLSampleCount[%d] ADSampleCount[%d]\n", g_szEdId, g_szPwd, g_upFlag, g_szVersion, g_iTTLSampleCount, g_iADSampleCount));

}

//--------------------------------END OF FILE---------------------------------------
