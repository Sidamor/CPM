#include <stdio.h>
#include <locale.h>
#include <time.h>

#include "TkMacros.h"
#include "Apm.h"
#include "Init.h"

/*Tcp server*/

CodecType DeCodeSVR(void* pMsg, void* pBuf, void* pRespBuf, int &RespLen, int& nUsed)
{
	MsgHdr *pMsgHdr = (MsgHdr *)pMsg;
	Msg *pTMsg = (Msg *)((char *)pMsg + MSGHDRLEN);
	// ????

	if(nUsed < (int)MSGHDRLEN )
	{
		return CODEC_NEED_DATA;
	}

	TKU32 unMsgLen = pMsgHdr->unMsgLen;
	TKU32 unMsgCode = pMsgHdr->unMsgCode;
	TKU32 unStatus  = pMsgHdr->unStatus;
	TKU32 unMsgSeq  = pMsgHdr->unMsgSeq;
    //DBG(("unmsgLen:[%d]\n", unMsgLen));
    //DBG(("unMsgCode:[%d]\n", unMsgCode));
	if (unMsgLen < MSGHDRLEN || unMsgLen > MAXMSGSIZE)
	{
        //DBG(("Code error\n"));
		return CODEC_ERR;
	}
	if(nUsed < (int)unMsgLen)
	{
		return CODEC_NEED_DATA;
	}

	nUsed = unMsgLen;
	if(unMsgCode & COMM_RSP)  // ??????
	{
		pMsgHdr = (MsgHdr *)pBuf;
		pMsgHdr -> unMsgLen = MSGHDRLEN; 
		pMsgHdr -> unMsgCode = unMsgCode;
		pMsgHdr -> unStatus = unStatus ;
		pMsgHdr -> unMsgSeq = unMsgSeq ;

		if(unStatus == 0x00)
			return CODEC_RESP;
		else
			return CODEC_ERR;
	}

	// ????
	pMsgHdr = (MsgHdr *)pRespBuf;
	pTMsg = (Msg *)((char *)pRespBuf + MSGHDRLEN);
	pMsgHdr -> unMsgLen = MSGHDRLEN; 
	pMsgHdr -> unMsgCode = unMsgCode|COMM_RSP;
	pMsgHdr -> unStatus = 0 ;
	pMsgHdr -> unMsgSeq = unMsgSeq ;

	//lhy ??????,?????
	

	// ?Msg?
	memcpy(pBuf, pMsg, unMsgLen );
	pMsgHdr = (MsgHdr *)pBuf;
	pTMsg = (Msg *)((char *)pBuf + MSGHDRLEN);
	
	if (unMsgCode==COMM_ACTIVE_TEST) // ?????????
	{	
		pMsgHdr->unMsgLen  = MSGHDRLEN;
		RespLen = MSGHDRLEN;
		//printf("Active Test ... ... ...\n");
		return CODEC_CMD;
	}

	return CODEC_CMD;
}

/*************************************************/
//Tcp client
bool LoginSvr(void *pSock)
{//?+STATUS(4)+PID(20)+CTIME(14)+MD5(32)
	ClsTCPClientSocket *m_Sock = (ClsTCPClientSocket*)pSock;

	char szBuf[512] = {0};
	MsgHdr* msgHdr = (MsgHdr*)szBuf;
	char* msg = (char*)msgHdr + MSGHDRLEN;

	unsigned char md5_input[256]={0};
	unsigned char md5_output[16]={0};
	char md5_hex[33] = {0};

    memset(g_szEdId, 0, sizeof(g_szEdId));
	memset(g_szPwd, 0, sizeof(g_szPwd));
    g_upFlag = 0;
	int RmTimeout = 1000;
//	get_ini_string("Config.ini","PARENT","RemoteLoginId" ,"=", g_szEdId, sizeof(g_szEdId));
//	get_ini_string("Config.ini","PARENT","RemoteLoginPwd" ,"=", g_szPwd, sizeof(g_szPwd));
//	get_ini_int("Config.ini","PARENT","RemoteTimeout", 0, &RmTimeout);
//    get_ini_int("Config.ini","PARENT","RemoteStatus", 0, &g_upFlag);

	int len=0, i=0, iIndex = 0;

	/* ??md5??? */
	i = 0;
	len = strlen(g_szEdId);			// ??????'\0'
	sprintf((char*)(md5_input+i), "%-20s", g_szEdId);	// sp_id 
	i += 20;

	time_t nowtime;
	time(&nowtime);
	
	//??????
	char szTime[15] = {0};
	Time2Chars(&nowtime, szTime);

	memcpy(md5_input+i, szTime, 14);	// gLogin_Pwd
	i += 14;

	len	= strlen(g_szPwd);		// ??????'\0'
	memcpy(md5_input+i, g_szPwd, len);	// gLogin_Pwd
	i += len;


	//AddMsg((unsigned char*)md5_input, i);
	MD5(md5_output, md5_input, i);		// ??md5??

	char* ptrMd5Hex = md5_hex;
	for(int i=0; i<16; i++)
	{
		sprintf(ptrMd5Hex, "%02X", md5_output[i]);
		ptrMd5Hex += 2;
	}	
	
    memcpy(msg + iIndex, "0000", 4);                    iIndex += 4;    // Status
	sprintf(msg + iIndex, "%-20s", g_szEdId);			iIndex += 20;	// EDID
	memcpy(msg + iIndex, szTime, 14);					iIndex += 14;	// Time
	memcpy(msg + iIndex, md5_hex, 32);					iIndex += 32;	// Md5
	
	msgHdr->unMsgLen = MSGHDRLEN + iIndex;
	msgHdr->unMsgCode = COMM_CONNECT;
	msgHdr->unMsgSeq = 1;
	msgHdr->unStatus = 0;

//	DBG(("LoginSvr AddMsg"));
//	AddMsg((unsigned char*)szBuf, msgHdr->unMsgLen);
	m_Sock->Send(szBuf, msgHdr->unMsgLen);
	if(true == m_Sock->IsPending(RmTimeout))
	{
		memset(szBuf,0,512);
	 	if(m_Sock->Recv(szBuf, 512) <= 0) 
		{
			return false;
		}

		if (msgHdr->unStatus ==0 )
		{
			m_Sock->GetPeer(szBuf, sizeof(szBuf));
		}
	}
	else
	{
		return false;
	}	

	return true;
}

int EnCodeCMPP(void* pMsg, void* pBuf)
{
	MsgHdr *pMsgHdr = (MsgHdr *)pMsg;
	MsgHdr* pCmppMsg = (MsgHdr*)pBuf;
	
	if( (pMsgHdr->unMsgCode & COMM_RSP) ||  pMsgHdr->unMsgCode==COMM_ACTIVE_TEST)	//???,???	
	{
		pCmppMsg->unMsgCode = pMsgHdr->unMsgCode;
		pCmppMsg->unMsgLen = pMsgHdr->unMsgLen - 4;
		pCmppMsg->unMsgSeq = pMsgHdr->unMsgSeq;
		pCmppMsg->unStatus = pMsgHdr->unStatus;
		return  pCmppMsg->unMsgLen;
	}

	memcpy(pBuf, pMsg, pMsgHdr->unMsgLen);

	return pMsgHdr->unMsgLen;
}

int EncodeMsgApp(BYTE* inbuf, BYTE* msg, int Cmd)
{
	return 0;
}

void MD5(unsigned char *szBuf, unsigned char *src, unsigned int len)
{
	MD5_CTX context;
	MD5Init(&context);
	MD5Update(&context, src, len);
	MD5Final(szBuf, &context);
}


bool SendMsgCommQ(TKU32 ulHandle, unsigned long ulMCBHandle, char *msg, BYTE sendLen )
{
	//PCOMM_MSG pMsgHdr = (PCOMM_MSG)msg;

	char *pMsg = (char *)MM_ALLOC(ulMCBHandle);
	if(NULL == pMsg)
	{
		DBG(("SendCommMsg MMAlloc_H failed  FILE=%s, LINE=%d hmsg[%u], hmem[%u]\n", __FILE__, __LINE__, ulHandle, (unsigned int)ulMCBHandle));
		return false;
	}
	DBG(("SendCommMsg MMAlloc_H FILE=%s, LINE=%d hmsg[%u], hmem[%u]\n", __FILE__, __LINE__, ulHandle, (unsigned int)ulMCBHandle));

	memcpy(pMsg, msg, sendLen);

	if(MMR_OK != g_MsgMgr.SendMsg(ulHandle,(QUEUEELEMENT)pMsg, 50))
	{
		DBG(("SendCommMsg failed hmsg[%u]Msgfull \n",  ulHandle));
		MM_FREE((QUEUEELEMENT&)pMsg );
		return false;
	}
	return true;
}

//?????? yyyymmddhhmmss
void SetSysTime(char* ptime)
{	
	//date 040720422008.45
	//MM DD hh mm YYYY ss
	char MM[3] = {0};
	char DD[3] = {0};
	char hh[3] = {0};
	char mm[3] = {0};
	char YYYY[5] = {0};
	char ss[3] = {0};
	memcpy(MM, ptime + 4, 2);
	memcpy(DD, ptime + 6, 2);
	memcpy(hh, ptime + 8, 2);
	memcpy(mm, ptime + 10, 2);
	memcpy(YYYY, ptime, 4);
	memcpy(ss, ptime + 12, 2);
	char datemsg[30] = {0};
	sprintf(datemsg, "date %2s%2s%2s%2s%4s.%2s", MM, DD, hh, mm, YYYY, ss);
	
	system(datemsg);
}

bool FileCopyB(char* srcfile, char* dstfile)
{
	bool ret = false;
	FILE* in,* out;
	char buffer[512+1];
	int n;
	do
	{
		in = fopen(srcfile,"rb+");
		if(!in)
			break;
		
		out = fopen(dstfile,"wb+");
		if(!out)
		{
			break;
		}
		
		while(!feof(in) && (n=fread(buffer,sizeof(char),512,in))>0)
			fwrite(buffer,sizeof(char),n,out);
		ret = true;
	}while(false);
	if(in)
		fclose(in);
	if(out)
		fclose(out);
	return ret;
}

int getpeeraddr(int sockfd, char* outBuf)
{
	char peeraddrstr[60] = {0};
	char peerip[18] = {0};
	struct sockaddr_in peeraddr;
	socklen_t len = sizeof(struct sockaddr_in);
	
	string timearg = gettime();
	int ret = getpeername(sockfd, (struct sockaddr *)&peeraddr, &len);
	if (ret < 0)
	{
		DBG(("getpeername failed[%d]\n", ret));
		return SYS_STATUS_FAILED;
	}
	sprintf(peeraddrstr, "time: %s\npeer address: %s:%d\n\n", timearg.c_str(), inet_ntop(AF_INET, &peeraddr.sin_addr, peerip, sizeof(peerip)), ntohs(peeraddr.sin_port));
	sprintf(outBuf, "%s", peerip);
	DBG(("%s------%s\n", outBuf, peeraddrstr));
	return SYS_STATUS_SUCCESS;
}

//?????????
string gettime()
{
	time_t t;
	struct tm *tm;
	char buf[20];

	t=time(NULL);
	tm=localtime(&t);
	snprintf(buf, 20, "%04d%02d%02d%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	string sendmsg(buf);
	return sendmsg;    
}

