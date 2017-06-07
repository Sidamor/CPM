#ifndef __CMPP_H__
#define __CMPP_H__

#include "Shared.h"
#include "Define.h"
#include "Init.h"


CodecType DeCodeSVR(void* pMsg, void* pBuf, void* pRespBuf, int &RespLen, int& nUsed);


bool LoginSvr(void* pSock);
int EnCodeCMPP(void* pMsg, void* pBuf);
void MD5(unsigned char *szBuf, unsigned char *src, unsigned int len);
bool SendMsgCommQ(TKU32 ulHandle, unsigned long ulMCBHandle, char *msg, BYTE sendLen);
void SetSysTime(char* ptime);
int EncodeMsgApp(BYTE* inbuf, BYTE* msg, int Cmd);//发送到网络的组包函数
bool FileCopyB(char* srcfile, char* dstfile);

int getpeeraddr(int sockfd, char* outBuf);
string gettime();
#endif
