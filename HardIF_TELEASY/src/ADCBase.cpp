#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>

#include "ADCBase.h"
#include "Shared.h"
#include "ADC_SPI.h"
#include "Define.h"

using namespace std;

#define SERIAL_SEND_LEN   (30)
#define SCTCONF_PATH	"/etc/sct.conf"

CADCBase::CADCBase(int mode)
{
	m_iMode = mode;
	m_hDevHandle = -1;
	memset((unsigned char*)m_ADPara, 0, 4*sizeof(float));
}

CADCBase::~CADCBase()
{
	Terminate();
}

void CADCBase::Terminate()
{
	if (0 <= m_hDevHandle)
	{
		ApiSpiClose(m_hDevHandle);
		m_hDevHandle = -1;
	}
}
bool CADCBase::Initialize(char* devName)
{	 
	DBG(("ADC设备[%s]初始化.", devName));
	m_hDevHandle = ApiSpiOpen(devName, O_RDWR);
	if (m_hDevHandle < 0)
	{
		DBG(("ApiSpiOpen[%s] failed\n", devName));
		return false;
	}
	DBG((" ."));
	ApiClearFIFO(m_hDevHandle);

	FILE *file = NULL;
	if ((file = fopen(SCTCONF_PATH, "r+")) == NULL)
	{
		return false;
	}
	DBG((" ."));
	char tempbuf[128] = {0};
	int i=0;
	bool bConfigGet = false;
	while(NULL != fgets(tempbuf, sizeof(tempbuf), file))
	{
		//printf("line:%s\n", tempbuf);
		if (1 == m_iMode && 3 == i)
		{
			bConfigGet = true;
			break;
		}
		else if (0 == m_iMode && 2 == i)
		{
			bConfigGet = true;
			break;
		}
		memset(tempbuf, 0, sizeof(tempbuf));
		i++;
	}
	fclose(file);
	file = NULL;

	DBG((" ."));
	
	if (!bConfigGet)
	{
		return false;
	}
	i = 0;
	char* ptrtemp = NULL;
	char* ptrdate = NULL;
	char* buf = tempbuf;
	printf("line:%s\n", buf);
	ptrdate = strtok_r(buf, ",", &ptrtemp);
	while(NULL != ptrdate)
	{
		m_ADPara[i] = atof(ptrdate);
		printf("para[%d] [%f] ",i, m_ADPara[i]);
		ptrdate = strtok_r(NULL, ",", &ptrtemp);
		i++;
	}
	if (4 != i)
	{
		DBG(("CADCBase::Initialize check paras count error! %d\n", i));
		return false;
	}
	DBG((" . 成功\n"));
	return true;
}

/************************************************************************/
/* Serial Operates                                                      */
/************************************************************************/
bool CADCBase::SetBaudRate(int interNum, ADC_BAUDRATE baudrate)
{
	int serial_index = 0;
	switch(interNum)
	{
	case INTERFACE_RS485_2:
	case INTERFACE_RS485_1:
		serial_index = 0;
		break;
	case INTERFACE_RS485_4:
	case INTERFACE_RS485_3:
		serial_index = 1;
		break;
	case INTERFACE_RS485_6:
	case INTERFACE_RS485_5:
		serial_index = 2;
		break;
	default:
		return false;
	}
    ApiWriteBaudRate(m_hDevHandle, (int)baudrate, serial_index);
	return true;
}

bool CADCBase::SetSerialOpt(int interNum, const int c_iStopBit, const int c_iOddEvent)
{
	int serial_index = 0;
	switch(interNum)
	{
	case INTERFACE_RS485_2:
	case INTERFACE_RS485_1:
		serial_index = 0;
		break;
	case INTERFACE_RS485_4:
	case INTERFACE_RS485_3:
		serial_index = 1;
		break;
	case INTERFACE_RS485_6:
	case INTERFACE_RS485_5:
		serial_index = 2;
		break;
	default:
		return false;
	}
	ApiWriteOthers( m_hDevHandle, c_iStopBit, c_iOddEvent, serial_index);
	ApiClearFIFONew(m_hDevHandle, serial_index);
	return true;
}

//串口数据读取
int CADCBase::SerialRecv(int interNum, unsigned char* outBuf, int needlen, int timeout)
{
    int recvLen = 0;
    int iLen = 0;
    int cnt = 0;
    DBG(("<--------COM RECV--------------[%d]--- timeout[%d]\n", interNum, timeout));

	int serial_index = 0;
	switch(interNum)
	{
	case INTERFACE_RS485_2:
	case INTERFACE_RS485_1:
		serial_index = 0;
		break;
	case INTERFACE_RS485_4:
	case INTERFACE_RS485_3:
		serial_index = 1;
		break;
	case INTERFACE_RS485_6:
	case INTERFACE_RS485_5:
		serial_index = 2;
		break;
	default:
		DBG(("SPI串口接收通道无效 通道号[%d]\n", interNum));
		return false;
	}
	
	bool readFirst = true;

	struct timeval tmval_Time;
	gettimeofday(&tmval_Time, NULL);

    do
    {
        iLen = ApiSerialRead(m_hDevHandle, outBuf+recvLen, needlen - recvLen, serial_index);
		if (iLen < 0)
        {
			DBG(("ApiSerialRead Failed iLen[%d]\n", iLen));
			break;
        }
		else if(0 == iLen)
		{
			if (!readFirst)
			{
				cnt++;
			}
		}
		else
		{
			readFirst = false;
		}
        recvLen += iLen;
		if (recvLen >= needlen)
		{
			break;
		}
		if (readFirst)
		{
			struct timeval tmval_NowTime;
			gettimeofday(&tmval_NowTime, NULL);
			long time_elipse = (tmval_NowTime.tv_sec - tmval_Time.tv_sec)*1000 + (tmval_NowTime.tv_usec - tmval_Time.tv_usec)/1000;
			if (time_elipse > timeout)
			{
				break;
			}
		}
		usleep(2000);
    }while( cnt <= 5);

	struct timeval tmval_NowTime;
	gettimeofday(&tmval_NowTime, NULL);
	unsigned long time_elipse = (tmval_NowTime.tv_sec - tmval_Time.tv_sec)*1000 + (tmval_NowTime.tv_usec - tmval_Time.tv_usec)/1000;

	DBG(("CommMsgRecive [%d] ret[%d] time[%ld] timeOut[%d]\n", interNum, recvLen, time_elipse, timeout));
    if( recvLen > 0 )
    {
        AddMsg(outBuf, recvLen);
    }
	return recvLen;
}

//串口数据写入
int CADCBase::SerialSend(int interNum, unsigned char* inBuf, int bufLen)
{
    int sendLen = 0;
	int len = 0;
    unsigned char sendBuf[SERIAL_SEND_LEN] = {0};
    DBG(("-------COM SEND---------------[%d]-->\n", interNum));
    AddMsg(inBuf, bufLen);

	int serial_index = 0;
	switch(interNum)
	{
	case INTERFACE_RS485_2:
	case INTERFACE_RS485_1:
		serial_index = 0;
		break;
	case INTERFACE_RS485_4:
	case INTERFACE_RS485_3:
		serial_index = 1;
		break;
	case INTERFACE_RS485_6:
	case INTERFACE_RS485_5:
		serial_index = 2;
		break;
	default:
		return false;
	}

    while( sendLen < bufLen )
    {
        memset(sendBuf, 0, sizeof(sendBuf));
        if( (sendLen + SERIAL_SEND_LEN) <= bufLen )         //需要发满30个字节
        {
            memcpy(sendBuf, &inBuf[sendLen], SERIAL_SEND_LEN);
            if( (len = ApiSerialWrite(m_hDevHandle, sendBuf, SERIAL_SEND_LEN, serial_index)) < 0 )
            {
                DBG(("Send To COM Failed!\n"));
				break;
            }
            else
            {
                sendLen += len;
            }
        }
        else                                                //剩下的数据不足30字节
        {
            memcpy(sendBuf, &inBuf[sendLen], (bufLen - sendLen) );
            if( (len = ApiSerialWrite(m_hDevHandle, sendBuf, (bufLen - sendLen), serial_index)) < 0 )
            {
                DBG(("Send To COM Failed!\n"));
				break;
            }
            else
            {
                sendLen += len;
//                DBG(("Send Coplete, Send Length:[%d]\n", sendLen));
            }
        }
    }
	return sendLen;
}

/************************************************************************/
/* AD 模拟量数据读取                                                    */
/************************************************************************/
bool CADCBase::ADGet(int AD_Index, float &outValue)
{
    float temp[20] = {0};
    int min = 0;
    int max = 0;
    for( int i=0; i<20; i++ )
    {
	    if (!ApiReadADC(m_hDevHandle, &temp[i], 1, AD_Index))
        {
		    DBG(("ADGet Failed: AD_Index[%d] m_hDevHandel[%d]\n", AD_Index, m_hDevHandle));
		    return false;
        }
        temp[i] *= m_ADPara[AD_Index];

        if( temp[i] < temp[min] )
        {
            min = i;
        }
        if( temp[i] > temp[max] )
        {
            max = i;
        }
    }
    temp[min] = 0;
    temp[max] = 0;
	outValue = 0;
	for (int i=0; i<20; i++)
	{ 
		outValue += temp[i];
	}
	outValue = (outValue)/18.0;
	return true;
}

/************************************************************************/
/* DI 开关量输入数据读取                                                */
/************************************************************************/
bool CADCBase::DIGet(int DI_Index, unsigned char &outValue)
{
	bool ret = false;
	unsigned char tempValue[20] = {0};
	for (int i = 0; i<20; i++)
	{
		if (!ApiReadGpio(m_hDevHandle, &(tempValue[i]), 1, DI_Index))
		{
			DBG(("DIGet Failed: DI_Index[%d] m_hDevHandel[%d]\n", DI_Index, m_hDevHandle));
			return ret;
		}
	}
	unsigned char temp = tempValue[0];
	for (int i=1; i<20; i++)
	{ 
		if (temp != tempValue[i])
		{
			return ret;
		}
	}
	outValue = temp;
	return true;
}

/************************************************************************/
/* DO 开关量输出数据设置                                                */
/************************************************************************/
void CADCBase::DOSet(int interNum, int outValue)
{
	int DO_Index = 0;
	switch(interNum)
	{
	case INTERFACE_DOUT_A1:
	case INTERFACE_DOUT_B1:
		DO_Index = 1;
		break;
	case INTERFACE_DOUT_A2:
	case INTERFACE_DOUT_B2:
		DO_Index = 2;
		break;
	case INTERFACE_DOUT_A3:
	case INTERFACE_DOUT_B3:
		DO_Index = 3;
		break;
	case INTERFACE_DOUT_A4:
	case INTERFACE_DOUT_B4:
		DO_Index = 4;
		break;
	default://灯控制
		DO_Index = interNum;
		break;
	}
	
	ApiWriteGpioGdo(m_hDevHandle, outValue, DO_Index);
}

/************************************************************************/
/* 继电器电源输出，由CADCBase的mode决定，如mode为1，则该函数有效        */
/************************************************************************/
void CADCBase::DOSetE(int DO_Index, int outValue)
{
	if (1 == m_iMode)
	{
		ApiWriteGpioGdo(m_hDevHandle, outValue, DO_Index);
	}
}

/*************************************************************************/
/*           模拟量口灯状态控制    0:亮灯     1:灭灯                     */
/*************************************************************************/
void CADCBase::DOAISet( int DO_Index, int status)
{
    ApiWriteGpioAI(m_hDevHandle, status, DO_Index);
}

/*************************************************************************/
/*           开关量口灯状态控制    0:亮灯     1:灭灯                     */
/*************************************************************************/
void CADCBase::DODISet( int DO_Index, int status)
{
    ApiWriteGpioDI(m_hDevHandle, status, DO_Index);
}

int CADCBase::f_getBaudRateEnum(unsigned int baudrate)
{
	if( baudrate == 1200 ){    return 1;}
	else if( baudrate == 2400 ){    return 2;}
	else if( baudrate == 4800 ){    return 3;}
	else if( baudrate == 9600 ){    return 4;}
	else if( baudrate == 19200 ){    return 5;}
	else if( baudrate == 38400 ){    return 6;}
	else if( baudrate == 57600 ){    return 9;}
	else if( baudrate == 115200 ){    return 8;}
	else{    return 4;}
}
