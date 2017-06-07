#ifndef _GSM_BASE_SIEMENS_
#define _GSM_BASE_SIEMENS_

#include <pthread.h>
#include <time.h>
#include <string>
#include "Define.h"
#include "QueueMgr.h"
#include "AudioPlay.h"

#define MAX_BUF_GSM_SIEMENS			1400			//

#define GPRS_LIVING_CHECK_TIME		200				//GPRS离线状态检测时间，如果在该时间段内无GPRS数据交互，则认为GPRS断开，重新上电GSM模块

#define SMS_RECV_INTERVAL			10				//SMS接收间隔时间
#define MAX_RECV_BUF_SIEMENS		2048

#define CMGL_PRE_LEN				15
#define MAX_MSG_LEN_GPRS			1024
typedef enum
{
	GPRS_NONE,
	GPRS_TCP_SVR,
	GPRS_TCP_CLI,
	GPRS_UDP_CLI,
	GPRS_FTP_CLI,
	GPRS_HTTP_CLI,
	GPRS_SMTP_CLI,
	GPRS_POP3_CLI,
}GPRS_INTERNET_SERVICE_TYPE;

//GSM模块命令信息分析结果
typedef enum{
	CODEC_SUCCESS_GSM,
	CODEC_ERR_GSM,
	CODEC_NEED_DATA_GSM,
	CODEC_PDU_TYPE_ERROR,
	CODEC_CODE_TYPE_ERROR
}CodecTypeGSM;

typedef enum{
	BIT7,
	BIT8,
	UCS2,
	UN_KONW
}CodecFormatGSM;

typedef enum _STA_GSM_SIEMENS
{
//	STA_INIT_SIEMENS,					//初始化中
	STA_LIVING_SIEMENS,					//已激活
	STA_CLOSED_SIEMENS,					//已关闭
	STA_HALTED_SIEMENS					//停止
}STA_GSM_SIEMENS;
 

void  getTimeStamp(char* pDst);
void  TelParityChange(char* pSrc, char* pDst);

typedef struct _INTERNET_SERVICE
{
	GPRS_INTERNET_SERVICE_TYPE ServiceType;
	char url[128];
	int status;
}INTERNET_SERVICE, *PINTERNET_SERVICE;

class CGSMBaseSiemens
{

public:
	CGSMBaseSiemens();
	virtual ~CGSMBaseSiemens();

	bool Initialize(char* comId, int baudrate, char* szSmsCenter, int ulHandleGSMSend, int ulHandleGSMRecv);

protected:	
	char m_strGprsIp[21];
	int m_iGprsPort;
	char m_strComId[56];
	int m_iBaudRate;
	char m_strIMSI[16];

//GSM module
public:
	static void* CheckThrd(void *arg);
	void CheckThrdProc(void);

	//lsd
	int GsmActionProc(SMS_INFO* pSms);
	int SetGsmSend(PGSM_MSG);
	int GetGsmRecv(GSM_MSG& outMsg);
	char* GetIMSI();
private:
	bool InitGSM();
	bool DoCmd(const char* ptrResq, char* checkValue, char* outBuf, int& outLen, int timeoutSec, int timeoutmill);
    bool DoCmdEx(const char* ptrResq, int reqLen, char* checkValue, char* outBuf, int& outLen, int timeoutSec, int timeoutmill);
	bool QueryAddr(char* outBuf);

	int GetSmsInfoByActionMsg(ACTION_MSG stAction, SMS_INFO& stSms);//动作消息转短信消息
//GPRS
public:
	int OpenSocket(GPRS_INTERNET_SERVICE_TYPE connectType, char* url);
	void CloseSocket(int handle);
	int SendGPRS(int handle, unsigned char* buf, int len);
	int RecvGPRS(int handle, unsigned char* buf, int len);

private:
	bool InitGPRS();
	bool OpenNetService(INTERNET_SERVICE netService, int svrId);
	bool CloseNetService(int svrId);

	int DoRecvGPRS(int handle, unsigned char* buf, int len);
	int DoSendGPRS(int handle, unsigned char* buf, int len);

//SMS
public:
	int SendSms(char* tel, char* content, int sn, int& seq);
	bool RecvSms(int smsType);
	bool DeleteSms(int index);


private:
	bool DoDeleteSms(int index);
	bool DoRecvSms(int smsType);

	unsigned char GetSmsId();
	int SendSmsPDU(char* tel, char* content, int& seq);
	int SendSmsPDU2(char* tel, char* content, int& seq);

	CodecTypeGSM ParseRecvPDU(char* pPDUMsg, SMS_INFO& smsOut);
	CodecTypeGSM DecodeRecvSMS(char* pPDUMsg, int& nUsed);

//outgoing calls
public:
	int PhoneCall(char* Tel, char* soundPos);
private:
	int DoPhoneCall(char* Tel, char* soundPos);

//Serial Port
private:
	bool OpenCom();
	int ComRecv(BYTE* oubBuf, int needLen, int timeoutSec, int timeoutmill, bool isPassive);
	bool ComSend(BYTE* msgSendBuf, BYTE sendLen);

private:
	class LockGSMBase
	{
	private:
		pthread_mutex_t	 m_lock;
	public:
		LockGSMBase() { pthread_mutex_init(&m_lock, NULL); }
		bool lock() { return pthread_mutex_lock(&m_lock)==0; }
		bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
	}CTblLockGSMBase;

private:
	unsigned char m_ucSmsId;
	GPRS_INTERNET_SERVICE_TYPE m_szGPRSServices[10];
	int m_hFd;

	STA_GSM_SIEMENS	m_enGSMStatus;		//GSM模块状态
	STA_GSM_SIEMENS m_enGPRS;			//GPRS模块状态
	STA_GSM_SIEMENS m_enSMS;			//短信模块状态
	STA_GSM_SIEMENS m_enPhone;			//电话模块状态

	time_t m_tmActivTimer;
	time_t m_tmRecvSms;

	pthread_t m_hCheckThrdHandle;		//状态检测线程句柄
	char m_szGsmCenter[21];

	//句柄
	int m_ulHandleGSMSend;
	int m_ulHandleGSMRecv;
	//SMS相关
	string m_strCenterAddr;//短信中心码
	int m_nSmsType;
	int m_nSmsCnt;

	int m_PidPlayAudio;
	int m_iWatchCnt;

	CAudioPlay m_AudioPlay;				//声音处理模块
};

#endif
