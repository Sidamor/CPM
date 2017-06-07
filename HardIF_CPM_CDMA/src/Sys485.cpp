/******************************************************************************/
/*    File    : switch_config.c                                               */
/*    Author  :                                                           */
/*    Creat   :                                                       */
/*    Function:                                                         */
/******************************************************************************/

/******************************************************************************/
/*                       头文件                                               */
/******************************************************************************/
#include <sys/time.h>
#include <sys/ioctl.h>
#include <string>
#include <stdlib.h>
#include <error.h>
#include "Shared.h"
#include "Sys485.h"

#include "Util.h"
#include "EmApi.h"



/******************************************************************************/
/*                       局部宏定义                                           */
/******************************************************************************/

/********************************************************************************/
/*                       局部函数												*/
/*参数定义																		*/
/*iChannelId:	串口号															*/
/*iBaudRate:	波特率															*/
/*iDataBytes:	数据位															*/
/*cCheckMode:	校验模式														*/
/*iStopBit:		停止位															*/
/*iReadTimeOut:	超时															*/
/*nHandle:		返回句柄（CloseComm用）											*/
/********************************************************************************/
int OpenComm(int iChannelId, int iBaudRate, int iDataBytes, char cCheckMode, int iStopBit, int iReadTimeOut, int& nHandle)
{
	char CommId[128] = {0};
	
	switch(iChannelId)
	{					
		//485接口
	case INTERFACE_RS485_1:
		strcpy(CommId, "/dev/ttyS2");
		break;
	case INTERFACE_RS485_2:
		strcpy(CommId, "/dev/ttyS3");
		break;
	default:
		return SYS_STATUS_ILLEGAL_CHANNELID;
	}

	nHandle = open(CommId, O_RDWR | O_NOCTTY | O_NDELAY);          //打开串口
	if (-1 == nHandle) 
	{
		fprintf (stderr, "cannot open port %s\n", CommId);
		return SYS_STATUS_OPEN_COMM_FAILED;
	}

	fcntl(nHandle, F_SETFL, 0);

	termios termios_new;
	// 0 on success, -1 on failure 
	bzero (&termios_new, sizeof (termios_new));
	cfmakeraw (&termios_new);

	int baudrate = BAUDRATE (iBaudRate);
	if(0 == baudrate)
		return SYS_STATUS_COMM_BAUDRATE_ERROR;
	
	termios_new.c_cflag = baudrate;
	termios_new.c_cflag |= CLOCAL | CREAD;      // | CRTSCTS 
	termios_new.c_cflag &= ~CSIZE;

	switch(iDataBytes)
	{
	case 5:
		termios_new.c_cflag |= CS5;
		break;
	case 6:
		termios_new.c_cflag |= CS6;
		break;
	case 7:
		termios_new.c_cflag |= CS7;
		break;
	default:
		termios_new.c_cflag |= CS8;
		break;
	}

	switch(cCheckMode)
	{
	case 'O'://奇校验
		termios_new.c_cflag |= PARENB;
		termios_new.c_cflag |= PARODD;
		termios_new.c_cflag |= (INPCK | ISTRIP);
		break;
	case 'E'://偶校验
		termios_new.c_cflag |= PARENB;
		termios_new.c_cflag &= ~PARODD;
		termios_new.c_cflag |= (INPCK | ISTRIP);
		break;
	default://无校验
		termios_new.c_cflag &= ~PARENB;
		break;
	}
    
    if(2 == iStopBit)                      //停止位
    {
        termios_new.c_cflag |= CSTOPB;              // 2 stop bit
    }
    else
    {
        termios_new.c_cflag &= ~CSTOPB;				// 1 stop bit
    }

	termios_new.c_oflag = 0;
	termios_new.c_lflag |= 0;
	//termios_new.c_oflag &= ~OPOST;
	termios_new.c_cc[VTIME] = 100;				// unit: 1/10 second. 
	termios_new.c_cc[VMIN] = 1;					// minimal characters for reading 
	tcflush (nHandle, TCIOFLUSH);

	if(0 != tcsetattr (nHandle, TCSANOW, &termios_new))
	{
		close(nHandle);
		nHandle = -1;
		return SYS_STATUS_COMM_PARA_ERROR;
	}	
	return SYS_STATUS_SUCCESS;	
}
int SendCommMsg(int nHandle, BYTE* msgSendBuf, int sendLen)
{
	int nMsgLeft = 0;
	int nMsgLen = 0;
	int nMaxResend = 0;
	BYTE MsgBuf[MAX_PACKAGE_LEN_TERMINAL] = {0};

	int nBytes = 0;
	DBG(("COMM SEND --Len[%d]----------------------------------->\n", sendLen));
	memcpy(MsgBuf, msgSendBuf, sendLen);
	nMsgLeft = nMsgLen = sendLen;
	if(nMsgLeft > 0)
	{ 		
		nMaxResend = 3;
		while (nMsgLeft && nMaxResend) 
		{
			nBytes = write(nHandle, MsgBuf + nMsgLen - nMsgLeft, nMsgLeft);
			if (nBytes < 0)
			{
				DBG(("SendMsg write < 0\n"));
				nMaxResend = -1;
				break;
			}
			AddMsg(MsgBuf + nMsgLen - nMsgLeft, nBytes);
			nMsgLeft = nMsgLeft - nBytes;
			nMaxResend--;
		}
	}
	return sendLen - nMsgLeft;
}
int RecvCommMsg(int nHandle, BYTE* msgRecvBuf, int iNeedLen, int iTimeOut)
{	
	int ret = 0;

	DBG(("COMM RECV ------------------------------------->\n"));
	fd_set ReadSetFD;
	FD_ZERO(&ReadSetFD);
	FD_SET(nHandle, &ReadSetFD);

	struct timeval stTimeOut;
	memset(&stTimeOut, 0, sizeof(struct timeval));

	stTimeOut.tv_sec = iTimeOut/1000;
	stTimeOut.tv_usec = (iTimeOut%1000)*1000;

	struct timeval tmval_Time;
	gettimeofday(&tmval_Time, NULL);
	
	//DBG(("sttimeval.tv_sec[%ld] sttimeval.tv_usec[%ld] nHandle[%d] iNeedLen[%d]\n", stTimeOut.tv_sec, stTimeOut.tv_usec, nHandle, iNeedLen));
	while (ret < iNeedLen)
	{
		if (0 >= select(nHandle + 1, &ReadSetFD, NULL, NULL, &stTimeOut))
		{
			DBG(("select <= 0\n"));
			break;
		}

		if (FD_ISSET(nHandle, &ReadSetFD)) 
		{
			int readlen = read(nHandle, msgRecvBuf + ret, iNeedLen - ret);
			ret += readlen;
		}
		stTimeOut.tv_sec = 0;
		stTimeOut.tv_usec = 50 * 1000;
	}

    if( 0 < ret )
    {
	    AddMsg(msgRecvBuf, ret);
    }
	else
	{
		DBG(("RecvTimeOut\n"));
		struct timeval tmval_NowTime;
		gettimeofday(&tmval_NowTime, NULL);
		long time_elipse = (tmval_NowTime.tv_sec - tmval_Time.tv_sec)*1000 + (tmval_NowTime.tv_usec - tmval_Time.tv_usec)/1000;
		DBG(("CommMsgRecive ret[%d] time[%ld] timeOut[%d]\n", ret, time_elipse, iTimeOut));
	}
    return ret;
}
void CloseComm(int iChannelId)
{
	if (-1 != iChannelId)
	{
		close(iChannelId);
	}
}
/*************************************  END OF FILE *****************************************/
