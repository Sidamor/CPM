/*****************************************************************************
*   程序名称: shared.cpp                                                     *
*   设 计 人: 吕丽民                                                         *
*   设计单位: 杭州百事达应用软件研究所                                       *
*   建立日期: 2003-06-22                                                     *
*   程序功能: 提供系统的共享函数                                             *
*   外 部 库:                                                                *
*   编译命令: 
g++ -shared -I./include -I./lib -L./lib  -lpthread -o ./lib/shared.so ./src/shared.cpp 
*   修 改 人: 吕丽民                                                         *   
******************************************************************************/
#include <locale.h>
#include <stdlib.h>
#include "Shared.h"
#include "MemMgmt.h"
#include "MsgMgrBase.h"

using namespace std;
#include <time.h>
#include <sys/timeb.h>
int gDataLogFlag;
bool gDataLogSockFlag;
char gDataLogFile[100];

pthread_mutex_t m_LockRecordFile;

/*************************************************************************
*       函数名称:       HexToInt()
*       函数功能:       将4个字节的字符串变成32位的整型数字:比如00a0变成160
*       输入参数:       str（4个字节的字符串）
*       输出参数:   result(str等价的数字）
*************************************************************************/
int HexToInt(string str)
{
	int result = 0; 
	sscanf( str.c_str(), "%x", &result );
	return result;
}

string IntToHex(int datalen)
{
	char result[4];
	int dd1,dd2,dd3;
	dd3=4096;
	for (int i=0;i<4;i++)
	{
		dd1=datalen/dd3;
		dd2=datalen%dd3;
		if (dd1>9)
			dd1=64+dd1-9;
		else
			dd1=48+dd1;
		result[i]=dd1;
		datalen=dd2;
		dd3=dd3/16;
	}
	result[4]=0;    
	string bb(result);
	return bb;
}
DWORD TurnDWord(const DWORD value)
{
	DWORD ret = 0;
	ret |= (value << 24) & 0xFF000000;
	ret |= (value <<  8) & 0x00FF0000;
	ret |= (value >>  8) & 0x0000FF00;
	ret |= (value >> 24) & 0x000000FF;
	return ret;
}

void AddMsg(unsigned char *str,int num)
{
	int i;
	DBG(("\n"));
	for (i = 0; i < num; i++)
	{
		DBG(("%02X ", str[i]));
		if (i % 4 == 3) DBG(("  "));
		if (i % 20 == 19) DBG(("\n"));
	}
	DBG(("\n"));
}

void AddMsg_NoSep(unsigned char *str,int num)
{
	int i;
	DBG(("\n"));
	for (i = 0; i < num; i++)
		DBG(("%02X", str[i]));
	DBG(("\n"));
}

bool ErrMsg(TKU32 ulHandle, unsigned long ulMCBHandle, char *msg)
{
	/*
	MsgHdr * pMsgHdr = (MsgHdr *)msg;
	Msg * pTMsg = (Msg *)(msg + MSGHDRLEN);
	
	time_t  timestamp=0;
	time(&timestamp);
	pMsgHdr -> tTimeStamp = timestamp;
	pTMsg->btType = FUNC_ERR;
	switch (pTMsg->stAlarm.btErrNo){
		case STA_TIMEOUT:
		case STA_BUSY:
		case STA_IF_DOWN:
		case STA_MEM_ALLOC_FAILED:
		case STA_SEND_FAILED:
		case STA_SYSTEM_ERR:
		case STA_UNKNOWN:
			sprintf(pTMsg -> stAlarm.szErrInfo, "%s","111");
			pTMsg -> stAlarm.btClass = ERR_GENERAL;
			break;
		default:
			break;
	}
	return SendMsg(ulHandle, ulMCBHandle, msg);
	*/
	return true;
}

bool SendMsg(TKU32 ulHandle, unsigned long ulMCBHandle, char *msg)
{
	MsgHdr * pMsgHdr = (MsgHdr *)msg;
	char *pMsg = (char *)MM_ALLOC(ulMCBHandle);
	if(NULL == pMsg)
	{
		AddMsg((unsigned char *)msg,pMsgHdr->unMsgLen);
		DBG(("SendMsg MMAlloc_H failed  FILE=%s, LINE=%d hmsg[%u], hmem[%u]\n", __FILE__, __LINE__, ulHandle, (unsigned int)ulMCBHandle));
		return false;
	}

	DBG(("SendMsg MMAlloc_H FILE=%s, LINE=%d hmsg[%u], hmem[%u]\n", __FILE__, __LINE__, ulHandle, (unsigned int)ulMCBHandle));

		
	memcpy(pMsg, msg, pMsgHdr->unMsgLen);
		
	if(MMR_OK != g_MsgMgr.SendMsg(ulHandle,(QUEUEELEMENT)pMsg, 50))
	{
		DBG(("SendMsg failed hmsg[%u]Msgfull \n",  ulHandle));
		MM_FREE((QUEUEELEMENT&)pMsg );
		return false;
	}
    //DBG(("Add to sendlist success\n"));

	return true;
}

bool SendMsg_Ex(TKU32 ulHandle, unsigned long ulMCBHandle, char *msg, int sendLen)
{
	TRACE(SendMsg_Ex);
    char *pMsg = (char *)MM_ALLOC(ulMCBHandle);
	if(NULL == pMsg)
	{
		DBG(("SendMsg_Ex MMAlloc_H failed  FILE=%s, LINE=%d hmsg[%u], hmem[%u]\n", __FILE__, __LINE__, ulHandle, (unsigned int)ulMCBHandle));
		return false;
	}
		
	DBG(("SendMsg_Ex MMAlloc_H FILE=%s, LINE=%d hmsg[%u], hmem[%u]\n", __FILE__, __LINE__, ulHandle, (unsigned int)ulMCBHandle));

	memcpy(pMsg, msg, sendLen);
	int t = 0;
    if(MMR_OK != (t = g_MsgMgr.SendMsg(ulHandle,(QUEUEELEMENT)pMsg, 50)))
	{
		DBG(("SendMsg_Ex failed hmsg[%u][%d]Msgfull \n",  ulHandle, t));
		MM_FREE((QUEUEELEMENT&)pMsg );
		return false;
	}
	return true;
}

#include <stdio.h>
extern int errno;
int writelog(char *path,string log)
{
	/*
	ofstream os(path,ios::app);
	time_t log_time;
	string str_log_time;

	time(&log_time);
	str_log_time = ctime(&log_time);
	
	os << str_log_time << log << endl;*/
	
	//printf("path:%s, data:%s\n", path, log.c_str());
	FILE *fs = fopen(path, "a");
	if (!fs)
	{ 
		//fprintf(stderr, "fopen failed, errno = %d (%s)\n", errno, strerror( errno)); 
		return -1;
	}
	fprintf(fs, "%s", log.c_str());
	fclose(fs);
	
	return 0;
}
int UnicodeToGB(BYTE* in, char* out)  // nicode转成GB, 返回GB字符个数, in,out均以0结束
{
	setlocale(LC_CTYPE, "zh_CN");
	int wcTotal = 0; // unicode×?·?êy
	int mbTotal = 0; // GB×?·?êy
	while (in[wcTotal*2]!=0 || in[wcTotal*2+1]!=0)
	{
		int k;  
		wchar_t wc = in[wcTotal*2+1] | ( ((unsigned short)in[wcTotal*2])<<8); // swap byte
		k = wctomb(out+mbTotal, wc);  // convert MB_CUR_MAX
		if (k>0)
			mbTotal += k;  
		++wcTotal; 
	}
	out[mbTotal] = '\0';
	return mbTotal;
}
bool GetApmId(char *dest,const char *src)
{		
	char szApmId[3] = {0};
	memcpy(szApmId, src, 2);
	int CPType = atoi(szApmId);
	switch(CPType)
	{
		case 	60:
		case 	61:
		case 	62:
		case 	63:
		case 	64:
		case 	65:
		case 	66:
		case  67:
		case  68:
		case  69:
		case 	71:
		  sprintf(dest,"%s",src);
		  break;
		default:
			sprintf(dest,"72%s",src);
			break;
	}
	return true;
			
}

bool GetRespId(BYTE *dst, BYTE *src, int pLen)
{
	int i = 0;
	BYTE ch;
	for(i = 0; i<pLen; i++)
	{
		ch = *src++;
		*dst = (BYTE)((ch >> 4) & 0x0F);
		if( *dst < 10)
			*dst += 0x30;
		else
			*dst += 0x37;
		dst++ ;
		*dst = (BYTE)(ch & 0x0F);
		if(*dst <10)
			*dst += 0x30;
		else
			*dst += 0x37;
		dst++;		
	}
	return true;
}
bool DataLogSock(BYTE pSrcType, char *pTitle, char *pSrc, int pLen)
{
		if(true == gDataLogSockFlag)
		{
			DataLog(pSrcType, pTitle, pSrc, pLen);
		}
		return true;	
}

bool DataLog(BYTE pSrcType, char *pTitle, char *pSrc, int pLen)
{
		string Str = pTitle ;
		Str += "\n";
		if(FUNC_STRINFO != pSrcType)
		{
			Str += MemToStr(pSrc, pLen);
		}
		else
		{
			Str += pSrc ;
		}
		Str += "\n";
		switch(gDataLogFlag)
		{
			case 0:
				printf("%s",Str.c_str());
				break;
			case 1:
				WriteDataLog(Str);
				break;
			default : 
				break;
		}			
		return true;	
}
int WriteDataLog(string pLog)
{
	FILE *fs = fopen(gDataLogFile, "a");
	if (!fs)
	{ 
		return -1;
	}
	fprintf(fs, "%s", pLog.c_str());
	fclose(fs);	
	return 0;
}

string MemToStr(char *pBuf, int pLen)
{
	string Str = "";
	int i;
	char Temp[20] = {0};

	for (i = 0; i < pLen; i++)
	{
		sprintf(Temp, "%02X ", ((unsigned char *)pBuf)[i]);
		Str += Temp;
		if (i % 4 == 3) Str += "  ";
		if (i % 20 == 19 && i != pLen-1) Str += "\n";
	}
	Str += "\n";
	return Str;
}

bool SendToOms(TKU32 pHandle, int pTBLMUCBEnum, bool pEnabled, BYTE pMod,char *pApmId, BYTE pMsgType, char *pSrc, int pLen)
{
	/*
	if(!pEnabled)
	{
		DataLog(pMsgType, pApmId, pSrc, pLen);
		return true;
	}
	char szBuff[MAXMSGSIZE];
	MsgHdr *pMsgHdr = (MsgHdr *)szBuff;
	Msg *pMsg = (Msg *)(szBuff + MSGHDRLEN);
	
	pMsgHdr -> unMsgLen = MSGHDRLEN + 28 + pLen;
	pMsgHdr -> unMsgCode = COMM_DELIVER;
	pMsgHdr -> unStatus = 0;
	pMsgHdr -> unMsgSeq = 0;
	
	memcpy(pMsg->szApmId, pApmId,21);
	pMsg->btModule = pMod;
	pMsg->btFuncNo = pMsgType;	
	pMsg->btStatus = STA_SUCCESS;	

	memcpy(pMsg->stMsgBuff.szBuff, pSrc, pLen);
	return SendMsg(pHandle, pTBLMUCBEnum, szBuff);
	*/
	return true;
}

void Time2Chars(time_t* t_time, char* pTime)
{
    int year=0,mon=0,day=0,hour=0,min=0,sec=0;

	time_t nowtime;
	struct tm *timeinfo;
	time(&nowtime);
	timeinfo = localtime(&nowtime);

	year = timeinfo->tm_year + 1900;
	mon = timeinfo->tm_mon + 1;
	day = timeinfo->tm_mday;
	hour = timeinfo->tm_hour;
	min = timeinfo->tm_min;
	sec = timeinfo->tm_sec;

    sprintf(pTime, "%04d-%02d-%02d %02d:%02d:%02d", year, mon, day, hour, min, sec);
}

string GetNowtimeStr()
{
	string ret = "";
	char pTime[15] = {0};
	int year=0,mon=0,day=0,hour=0,min=0,sec=0;

	time_t nowtime;
	struct tm *timeinfo;
	time(&nowtime);
	timeinfo = localtime(&nowtime);

	year = timeinfo->tm_year + 1900;
	mon = timeinfo->tm_mon + 1;
	day = timeinfo->tm_mday;
	hour = timeinfo->tm_hour;
	min = timeinfo->tm_min;
	sec = timeinfo->tm_sec;

	sprintf(pTime, "%04d%02d%02d%02d%02d%02d", year, mon, day, hour, min, sec);
	
	ret = pTime;
	return ret;	
}

void Str2time(char* strTime, time_t* ntime)
{
    struct tm m_tm;
    char temp[5] = {0};

    memcpy( temp, strTime, 4 );
    m_tm.tm_year = atoi(temp) - 1900;

    memset(temp, 0, sizeof(temp));
    memcpy( temp, strTime+5, 2 );
    m_tm.tm_mon = atoi(temp) - 1;

    memset(temp, 0, sizeof(temp));
    memcpy( temp, strTime+8, 2 );
    m_tm.tm_mday = atoi(temp);

    memset(temp, 0, sizeof(temp));
    memcpy( temp, strTime+11, 2 );
    m_tm.tm_hour = atoi(temp);

    memset(temp, 0, sizeof(temp));
    memcpy( temp, strTime+14, 2 );
    m_tm.tm_min = atoi(temp);

    memset(temp, 0, sizeof(temp));
    memcpy( temp, strTime+17, 2 );
    m_tm.tm_sec = atoi(temp);

    *ntime = mktime(&m_tm);
}

string BytesToStr(BYTE *pBuf, int pLen)
{
	string Str = "";
	int i;
	char Temp[20] = {0};

	for (i = 0; i < pLen; i++)
	{
		sprintf(Temp, "%02X", ((unsigned char *)pBuf)[i]);
		Str += Temp;
	}
	return Str;
}

short ByteToShort(BYTE *pBuf)
{
	short tmpShort = 0;
	memcpy((unsigned char*)&tmpShort, pBuf, sizeof(short));
	return tmpShort;
}

void strRightFillSpace(const char* src, int inLen,  char* out, int outlen)
{
	int i;
	strncpy(out, src, inLen);
	for(i=0; i<(outlen - inLen); i++)
	{
		out[inLen + i] = ' ';
	}
}

char* ltrim(char* s, int len)
{
	const char* space = " "; 
	char ret[2] = {0};
	int i = 0;
	ret[0] = s[0];
	while(NULL != strstr(space, ret) && i<len)
	{
		i++;
		ret[0] = s[i];
	}
	return (s + i);
}
char* rtrim(char* s, int len)
{
	const char* space = " "; 
	char ret[2] = {0};
	int i=len-1;
	ret[0] = s[i];
	while(NULL != strstr(space, ret) && i > 0)
	{		
		s[i] = '\0';
		i--;
		ret[0] = s[i];
	}	
	return s;
}
char* trim(char* s, int len)
{
	char* tmp = ltrim(s,len);
	tmp = rtrim(tmp, strlen(tmp));
	return tmp;
}
bool filecopy(char* srcfile, char* dstfile)
{
	bool ret = false;
	FILE* in,* out;
	char buffer[512+1];
	int n;
	do
	{
		in = fopen(srcfile,"r");
		out = fopen(dstfile,"w+");
		if(!in || !out)
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

TKU16 ConvertUShort(TKU16 value)
{
	TKU16 ret;
	ret = value << 8;
	ret |= value >> 8;
	return ret;
}

void DEBUG_LOG(char* pStr)
{
#ifdef DEBUG
	time_t log_time;
	string str_log_time;

	time(&log_time);
	str_log_time = ctime(&log_time);
	DBG(("%s: %s", str_log_time.c_str(), pStr));
#endif
}

//********字符串转16进制
int  String2Hex(unsigned char* pDest, unsigned char* pSrc, int iLenSrc)
{
	int iCount, iResult =0, nIndex =0;
	if ((!pSrc) || (!pDest) || (iLenSrc <=0)) 
		return iResult;
	
	for(iCount =0; iCount <iLenSrc; iCount+=2)
	{
		// 输出高4位
		if(pSrc[iCount]>='0'&&pSrc[iCount]<='9')
		{
			pDest[nIndex] = pSrc[iCount] - 0x30;
			pDest[nIndex] <<= 4;
		}
		else if (pSrc[iCount]>='A'&&pSrc[iCount]<='F')
		{
			pDest[nIndex] = pSrc[iCount] - 0x37;
			pDest[nIndex] <<= 4;
		}
		else if (pSrc[iCount]>='a'&&pSrc[iCount]<='f')
		{
			pDest[nIndex] = pSrc[iCount] - 0x57;
			pDest[nIndex] <<= 4;
		}
        else 
			return iResult;
		
		// 输出低4位
		if(pSrc[iCount+1]>='0'&&pSrc[iCount+1]<='9')
		{
			pDest[nIndex] |= pSrc[iCount+1] -0x30;
		}
		else if (pSrc[iCount+1]>='A'&&pSrc[iCount+1]<='F')
		{
			pDest[nIndex] |= pSrc[iCount+1] -0x37;
		}
		else if (pSrc[iCount+1]>='a'&&pSrc[iCount+1]<='f')
		{
			pDest[nIndex] |= pSrc[iCount+1] -0x57;
		}
        else 
			return iResult;
		
		nIndex++;
	}
	iResult =(nIndex>0) ? nIndex : 0;
	return iResult;
}


//********16进制转字符串
int  Hex2String(char* pSrc, char* pDest, int iLenDest)
{
	int iCount;
    char temp,temp1 = 0;
	if ((!pSrc) || (!pDest) || (iLenDest <=0)) 
		return 0;

    for( iCount = 0; iCount < iLenDest; iCount+=2 )
    {
        //sscanf(pDest, "%x ", &temp);
        memcpy( &temp, pDest, 1 );
        if( (temp >> 4) <= 9 )
        {
            temp1 = (temp >> 4) + 0x30;
            //sprintf(pSrc, "%d ", (int)(temp >> 8) );
            memcpy( pSrc, &temp1, 1);
        }
        else
        {
            temp1 = (temp >> 4) + 0x37;
            //sprintf(pSrc, "%c ", (char)(temp >> 8) );
            memcpy( pSrc, &temp1, 1);
        }
        pSrc++;
        if( (temp & 0x0F) <= 9 )
        {
            temp1 = (temp & 0x0F) + 0x30;
            //sprintf(pSrc, "%d ", (int)(temp & 0x0F) );
            memcpy( pSrc, &temp1, 1);
        }
        else
        {
            temp1 = (temp & 0x0F) + 0x37;
            //sprintf(pSrc, "%c ", (char)(temp & 0x0F) );
            memcpy( pSrc, &temp1, 1);
        }
        pDest++;
        pSrc++;
    }
    return 1;
	
}

/******************************************************************************/
/* Function Name  : transPolish                                               */
/* Description    : 计算式转换成逆波兰式                                      */
/* Input          : szRdCalFunc     计算式                                    */
/* Output         : tempCalFunc     逆波兰式                                  */
/* Return         :                                                           */
/******************************************************************************/
void transPolish(char* szRdCalFunc, char* tempCalFunc)
{
	char stack[128] = {0};
	char ch;
	int i=0,t=0,top = 0;

	ch = szRdCalFunc[i];
	i++;

	while( ch != 0)          //计算式结束
	{
		switch( ch )
		{
		case '+':
		case '-': 
			while(top != 0 && stack[top] != '(')
			{
				tempCalFunc[t] = stack[top];
				top--;
				t++;
			}
			top++;
			stack[top] = ch;
			break;
		case '*':
		case '/':
			while(stack[top] == '*'|| stack[top] == '/')
			{
				tempCalFunc[t] = stack[top];
				top--;
				t++;
			}
			top++;
			stack[top] = ch;
			break;
		case '(':
			top++;
			stack[top] = ch;
			break;
		case ')':
			while( stack[top]!= '(' )
			{
				tempCalFunc[t] = stack[top];
				top--;
				t++;
			}
			top--;
			break;
		case ' ':break;
		default:
			while( isdigit(ch) || ch == '.')
			{
				tempCalFunc[t] = ch;
				t++;
				ch = szRdCalFunc[i];
				i++;
			}
			i--;
			tempCalFunc[t] = ' ';	//数字之后要加空格，以避免和后面的数字连接在一起无法正确识别真正的位数
			t++;
		}
		ch = szRdCalFunc[i];
		i++;
	}
	while(top!= 0)
	{
		tempCalFunc[t] = stack[top];
		t++;
		top--;
	}
	tempCalFunc[t] = ' ';
}

/******************************************************************************/
/* Function Name  : compvalue                                                 */
/* Description    : 逆波兰式计算结果                                          */
/* Input          : tempCalFunc     逆波兰式                                  */
/*                  midResult       计算用数值                                */
/* Output         :                                                           */
/* Return         : 计算结果                                                  */
/******************************************************************************/
float compvalue(char* tempCalFunc, float* midResult)
{
	float stack[20];
	char ch = 0;
	char str[100] = {0};
	int i=0, t = 0,top = 0;
	ch = tempCalFunc[t];t++;

	while(ch!= ' ')
	{
		switch(ch)
		{
		case '+':
			stack[top-1] = stack[top-1] + stack[top];
			top--;
			break;
		case '-':
			stack[top-1] = stack[top-1] - stack[top];
			top--;
			break;
		case '*':
			stack[top-1] = stack[top-1] * stack[top];
			top--;
			break; 
		case '/':
			if(stack[top]!= 0)
				stack[top-1] = stack[top-1]/stack[top];
			else
			{
				DBG(("除零错误！\n"));
				break;
			}
			top--;
			break;
		default:
			i = 0;
			while( isdigit(ch) || ch == '.')
			{
				str[i] = ch;
				i++;
				ch = tempCalFunc[t];
				t++;
			}
			str[i] = '\0';
			top++;
			stack[top] = midResult[atoi(str)];

		}
		ch = tempCalFunc[t];
		t++;
	}
	return stack[top];
}
