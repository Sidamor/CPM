#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "LabelIF.h"
#include "Shared.h"
#include "Define.h"
#include "Crc.h"
#include "CodeUtil.h"
using namespace std;

#ifdef  DEBUG
#define DEBUG_BESTLABLE
#endif

#ifdef DEBUG_BESTLABLE
#define DBG_LB(a)		printf a;
#else
#define DBG_LB(a)	
#endif

/******************************************************************************/
/* Function Name  : GetLable                                             */
/* Description    : 编码发往串口的数据                                        */
/* Input          : char* format         格式化字符串                               */
/*                  char* lable          标签名                             */
/* Output         : int&  len			 标签长度		                   */
/*                  char* outBuf         标签内容                             */
/* Return         : char* formatPos      标签起始地址                         */
/******************************************************************************/
char* GetLable(char* format, char* lableName, int& len, char* outBuf)
{
	char lable[64] = {0};
	sprintf(lable, "{%s:", lableName);
	char* ret = strstr(format, lable);
	if (NULL == ret)
	{
		return NULL;
	}

	char sscanfFormat[64] = {0};
	sprintf(sscanfFormat, "%s%%[^}]",lable);
	if (0 >= sscanf(ret, sscanfFormat,outBuf))
	{
		return NULL;
	}

	len = strlen(lable) + strlen(outBuf) + 1;
	return ret;
}

char* GetCalLable(char* format, char* lableName, int sublableName, int& len, char* outBuf, unsigned char* tableType)
{
    DBG_LB(("Compare Table:format[%s] lableName[%s] sublableName[%d]\n", format, lableName, sublableName));
    if( format[1] == 'S' )         //查询表
    {
        *tableType = 1;
    }
    else if( format[1] == 'R' )    //斜率表
    {
        *tableType = 2;
    }
	char lable[64] = {0};
	sprintf(lable, "{%s:", lableName);
	char* ret = strstr(format, lable);      //找到大括号的位置
	if (NULL == ret)
	{
		return ret;
	}

	char sscanfFormat[64] = {0};
    char midRet[128] = {0};
	sprintf(sscanfFormat, "%s%%[^}]",lable);
	if (0 >= sscanf(ret, sscanfFormat,midRet))    //将{}间内容读取到midRet中
	{
		return ret;
	}
    DBG_LB(("midRet:[%s]\n", midRet));

    memset(lable, 0, sizeof(lable));
    ret = NULL;
    sprintf(lable, "[%02d:", sublableName);
	ret = strstr(midRet, lable);      //找到中括号的位置
	if (NULL == ret)
	{
		return ret;
	}

    memset(sscanfFormat, 0, sizeof(sscanfFormat));
    sprintf(sscanfFormat, "%s%%[^]]",lable);
	if (0 >= sscanf(ret, sscanfFormat,outBuf))    //将[xx:]间内容读到outBuf中
	{
		return ret;
	}
    DBG_LB(("outBuf:[%s]\n", outBuf));

	len = strlen(lable) + strlen(outBuf) + 1;
	return ret;
}

int GetCheckValue(const char* split, char* Inbuf, PFORMAT_CHECK pFormatCheck)
{
    char* paraType = NULL;
    char* stok = NULL;
    char* savePtr = NULL;
    char tempFormat[MAX_PACKAGE_LEN_TERMINAL] = {0};
    memset((unsigned char*)pFormatCheck, -1, sizeof(FORMAT_CHECK));
    memcpy(tempFormat, Inbuf+1, strlen(Inbuf)-2);

    //DBG_LB(("temp:[%s]\n", tempFormat));
    stok = strtok_r( tempFormat, split, &savePtr);
    while( stok != NULL )
    {
        //DBG_LB(("stok:[%s]\n", stok));
        paraType = NULL;
        if( NULL != (paraType = strstr(stok, "DS=")) )
        {
            pFormatCheck->DS = atoi(stok + strlen("DS="));
        }
        else if( NULL != (paraType = strstr(stok, "DE=")) )
        {
            pFormatCheck->DE = atoi(stok + strlen("DE="));
        }
        else if( NULL != (paraType = strstr(stok, "CS=")) )
        {
            pFormatCheck->CS = atoi(stok + strlen("CS="));
        }
        else if( NULL != (paraType = strstr(stok, "CE=")) )
        {
            pFormatCheck->CE = atoi(stok + strlen("CE="));
        }
        else if( NULL != (paraType = strstr(stok, "CL=")) )
        {
            pFormatCheck->CL = atoi(stok + strlen("CL="));
        }
        else if( NULL != (paraType = strstr(stok, "T=")) )
        {
            pFormatCheck->T = atoi(stok + strlen("T="));
        }
        else if( NULL != (paraType = strstr(stok, "C=")) )
        {
            pFormatCheck->C = atoi(stok + strlen("C="));
        }
        stok = strtok_r( NULL, split, &savePtr);
    }
    return 1;
}

//取得头部定义(数据类型，码流类型)
int GetHeadValue(const char* split, char* Inbuf, PFORMAT_HEAD pFormatHead)
{
    char* paraType = NULL;
    char* stok = NULL;
    char* savePtr = NULL;
    char tempFormat[MAX_PACKAGE_LEN_TERMINAL] = {0};
    memset((unsigned char*)pFormatHead, -1, sizeof(FORMAT_HEAD));
    memset(pFormatHead->DT, 0, sizeof(pFormatHead->DT));
    memcpy(tempFormat, Inbuf, strlen(Inbuf));

    stok = strtok_r( tempFormat, split, &savePtr);
    while( stok != NULL )
    {
        paraType = NULL;
        if( NULL != (paraType = strstr(stok, "D=")) )       //取数据类型
        {
            strcpy(pFormatHead->DT, stok+ strlen("D=")); 
            //DBG_LB(("DT:[%s]\n", pFormatHead->DT));
        }
        else if( NULL != (paraType = strstr(stok, "M=")) )       //取码流
        {
            pFormatHead->ST = atoi(stok+ strlen("M="));    
		} 
		else if( NULL != (paraType = strstr(stok, "DT=")) )       //值类型lhy
		{
			pFormatHead->T = atoi(stok+ strlen("DT="));    
		}
        stok = strtok_r( NULL, split, &savePtr);
    }

    return 1;
}

int GetValue(const char* split, char* Inbuf, PFORMAT_PARA pFormatPara)//L=1,C=1
{
	char tempFormat[128] = {0};
	memcpy(tempFormat, Inbuf, strlen(Inbuf));
	char* savePtr = NULL;
	char* token = strtok_r( tempFormat, split, &savePtr);

	memset((unsigned char*)pFormatPara, -1, sizeof(FORMAT_PARA));
    memset(pFormatPara->DC, 0, sizeof(pFormatPara->DC));
	while( NULL != token )//lhy 需要多字节表示？
	{
        //DBG_LB(("toke:[%s]\n", token));
		char* paraType = NULL;
		if ( (NULL != (paraType = strstr(token, "L="))) && (paraType[0] == token[0]) )
		{
            //DBG_LB(("1"));
			pFormatPara->DataLen = atoi(paraType + strlen("L="));    //截取数据长度
		}
        else if (NULL != (paraType = strstr(token, "LT=")))
        {
            //DBG_LB(("1"));
			pFormatPara->LenType = atoi(paraType + strlen("LT="));    //截取数据长度
        }
		else if ( (NULL != (paraType = strstr(token, "C="))) && (paraType[0] == token[0]) )
		{
            //DBG_LB(("2"));
			pFormatPara->ChangeBytes = atoi(paraType+ strlen("C="));     //高低字节转换
		}
		else if ( (NULL != (paraType = strstr(token, "T="))) && (paraType[0] == token[0]) )
		{
            //DBG_LB(("3"));
			pFormatPara->DataType = atoi(paraType+ strlen("T="));     //数值类型
		}
		else if (NULL != (paraType = strstr(token, "S=")))
		{
            //DBG_LB(("4"));
			pFormatPara->StartBytes = atoi(paraType+ strlen("S="));     //开始截取位置
		}
		else if (NULL != (paraType = strstr(token, "E=")))
		{
            //DBG_LB(("5"));
			pFormatPara->EndBytes = atoi(paraType+ strlen("E="));
		}
        else if (NULL != (paraType = strstr(token, "RT=")))
		{
            //DBG_LB(("6"));
			pFormatPara->RT = atoi(paraType+ strlen("RT="));    //系数类型
		}
        else if (NULL != (paraType = strstr(token, "RL=")))
		{
            //DBG_LB(("7"));
			pFormatPara->RL = atoi(paraType+ strlen("RL="));    //系数长度
		}
        else if (NULL != (paraType = strstr(token, "R=")))
		{
            //DBG_LB(("8"));
			pFormatPara->R = atof(paraType+ strlen("R="));      //系数
		}
        else if (NULL != (paraType = strstr(token, "RST=")))
        {
            //DBG_LB(("9"));
            pFormatPara->RST = atoi(paraType+ strlen("RST="));  //系数数值类型
        }
        else if (NULL != (paraType = strstr(token, "DC=")))
        {
            //DBG_LB(("11"));
            strcpy(pFormatPara->DC, paraType+ strlen("DC="));   //数据计算方法
        }
		else if (NULL != (paraType = strstr(token, "BP=")))
        {
            //DBG_LB(("9"));
            pFormatPara->BitPos = atoi(paraType+ strlen("BP="));  //系数数值类型
        }
		token = strtok_r( NULL, split, &savePtr);
	}

	return 1;
}

int GetValueTime(const char* Type, char* Inbuf, PFORMAT_TIME pFormatTime)
{
    char tempFormat[MAX_PACKAGE_LEN_TERMINAL] = {0};
	memcpy(tempFormat, Inbuf, strlen(Inbuf));
	char* savePtr = NULL;
	char* token = strtok_r( tempFormat, ",", &savePtr);

	memset((unsigned char*)pFormatTime, -1, sizeof(FORMAT_TIME));
    while( NULL != token )
	{
		char* paraType = NULL;
		if (NULL != (paraType = strstr(token, "TI=")))
		{
			pFormatTime->TI = atoi(paraType + strlen("TI="));    //是否有时间标记
		}
		else if (NULL != (paraType = strstr(token, "M=")))
		{
			pFormatTime->M = atoi(paraType+ strlen("M="));       //数据码流
		}
        else if (NULL != (paraType = strstr(token, "Y=")))
		{
			pFormatTime->Y = atoi(paraType+ strlen("Y="));       //年
		}
        else if (NULL != (paraType = strstr(token, "MO=")))
		{
			pFormatTime->MO = atoi(paraType+ strlen("MO="));     //月
		}
        else if (NULL != (paraType = strstr(token, "D=")))
		{
			pFormatTime->D = atoi(paraType+ strlen("D="));       //日
		}
        else if (NULL != (paraType = strstr(token, "H=")))
		{
			pFormatTime->H = atoi(paraType+ strlen("H="));       //时
		}
        else if (NULL != (paraType = strstr(token, "MI=")))
		{
			pFormatTime->MI = atoi(paraType+ strlen("MI="));     //分
		}
        else if (NULL != (paraType = strstr(token, "S=")))
		{
			pFormatTime->S = atoi(paraType+ strlen("S="));       //秒
		}
        token = strtok_r( NULL, ",", &savePtr);
	}

	return 1;
}

int GetValueByTable(unsigned char calType, float calValue, unsigned char tableType, void* curValue, PFORMAT_PARA rdFormat, FORMAT_HEAD headFormat, PFORMAT_MAP_TABLE mapTable, int mapSize, float* outValue)
{
	int ret = SYS_STATUS_FAILED;

    double valueD = 0.0;
    if( calType == 1 )       //常量查表
    {
        valueD = calValue;
    }
    else                     //截取值查表
    {
	    char data[256] = {0};
		DBG_LB(("GetValueByTable rdFormat->RST[%d] headFormat.ST[%d]\n", rdFormat->RST, headFormat.ST));
	    memcpy(data, (char*)curValue + (int)rdFormat->R, rdFormat->RL);
	    //AddMsg((unsigned char*)curValue+(int)rdFormat->R, rdFormat->RL);
	    switch(rdFormat->RST)
        {
	    case INTEGER:
            {
			    int iValue = 0; 
			    if (headFormat.ST == STREAM_CHARS)//字符流
                {
				    valueD = ((double)atoi(data));
                }
			    else
                {
				    iValue = *((int*)data);
				    if (1 == rdFormat->ChangeBytes)
                    {
					    iValue = TurnDWord(iValue);
                    }
				    valueD = ((double)iValue);
                }
            }
		    break;
	    case SHORT:
            {
			    short snValue = 0; 
			    if (headFormat.ST == STREAM_CHARS)//字符流
                {
				    valueD = ((double)atoi(data));
                }
			    else
                {
				    snValue = *((short*)data);
				    if (1 == rdFormat->ChangeBytes)
                    {
					    snValue = ConvertUShort(snValue);
                    }
				    valueD = ((double)snValue);
                }
            }
		    break;
	    case FLOAT:
		    if (headFormat.ST == STREAM_CHARS)//字符流
			    valueD = atof(data);
		    else
			    valueD = *((float*)data);
		    break;			
	    case CHAR:
		    if (headFormat.ST == STREAM_CHARS)//字符流
		    	valueD = ((double)atoi(data));
		    else
			    valueD = (double)(*((char*)data));
		    sprintf((char*)outValue, "%.2f", valueD);
		    break;
	    case CHARS:
	    case BYTES:
		    memcpy(outValue, data, rdFormat->RL);
		    return SYS_STATUS_SUCCESS;
        }
    }
    DBG_LB(("Table Cal ValueD:[%.2f]   mapSize:[%d]\n", valueD, mapSize));

    if( tableType == 1 )          //匹配表
    {
        for (int i=0; i<mapSize; i++)
        {
            DBG_LB(("ValueD:[%.2f]  ValueX:[%.2f]\n", valueD, mapTable[i].ValueX));
            if( valueD == mapTable[i].ValueX )             //依次查询表中内容
            {
                //sprintf((char*)outValue, "%.2f", mapTable[i].ValueY);
                *outValue = mapTable[i].ValueY;
				ret = SYS_STATUS_SUCCESS;
				DBG_LB(("outValue[%f] mapTable[%d].ValueY[%f]\n", *outValue, i, mapTable[i].ValueY));
				break;
            }
        }
    }
    else if( tableType == 2 )     //斜率表
    {
    	for (int i=0; i<mapSize; i++)
        {
            DBG_LB(("valueX:[%.2f]  valueY:[%.2f]\n", mapTable[i].ValueX, mapTable[i].ValueY));
			if (mapTable[i].ValueX == valueD)              //直接相等，取标准值
            {
			    valueD = mapTable[i].ValueY;
			    *outValue = valueD;
			    ret = SYS_STATUS_SUCCESS;
			    break;
            }
		    else if (1 == mapSize)
            {
			    valueD = valueD*mapTable[i].ValueY/mapTable[i].ValueX;
			    //sprintf((char*)outValue, "%.2f", valueD);
                *outValue = valueD;
			    ret = SYS_STATUS_SUCCESS;
			    break;
            }
		    else if ( 0==i &&                              //小于最小标准或大于最大标准
			    ((mapTable[i].ValueX >= mapTable[i+1].ValueX && valueD >= mapTable[i].ValueX)
				    || (mapTable[i].ValueX <= mapTable[i+1].ValueX && valueD <= mapTable[i].ValueX)))
            {
			    valueD = valueD*mapTable[i].ValueY/mapTable[i].ValueX;
			    *outValue = valueD;
			    ret = SYS_STATUS_SUCCESS;
			    break;
            }
		    else if ((mapSize - 1) == i)                   //最后的标准
            {
			    valueD = valueD*mapTable[i].ValueY/mapTable[i].ValueX;
			    *outValue = valueD;
			    ret = SYS_STATUS_SUCCESS;
			    break;
            }
		    else if ((mapTable[i].ValueX >= valueD && mapTable[i + 1].ValueX <= valueD)       //正常斜率
			    || (mapTable[i].ValueX <= valueD && mapTable[i + 1].ValueX >= valueD))
            {
				DBG_LB(("Standard "));
			    valueD = (mapTable[i+1].ValueY - mapTable[i].ValueY)*(valueD- mapTable[i].ValueX)/(mapTable[i+1].ValueX - mapTable[i].ValueX) + mapTable[i].ValueY;
			    *outValue = valueD;
			    ret = SYS_STATUS_SUCCESS;
			    break;
            }
        }
    }
	return ret;
}

int GenerateTable(const char* split1, const char* split2, char* inData, PFORMAT_MAP_TABLE mapTable, int size)//L=1,C=1
{
	int ret = 0;

	char tempFormat[4096] = {0};
	memcpy(tempFormat, inData, strlen(inData));

	char sscanfFormat[64] = {0};
	sprintf(sscanfFormat, "%%f%s%%f", split2);
	
	char* savePtr = NULL;
	char* token = strtok_r( tempFormat, split1, &savePtr);
	if (NULL == token)            //?
	{
		sscanf(tempFormat, sscanfFormat, &mapTable[ret].ValueX, &mapTable[ret].ValueY);
	}
	else
	{
		while( token != NULL)
		{
			float tmpFloat1, tmpFloat2;
			sscanf(token, sscanfFormat, &tmpFloat1, &tmpFloat2);
			mapTable[ret].ValueX = tmpFloat1;
			mapTable[ret].ValueY = tmpFloat2;
			DBG_LB(("mapTable[%d].ValueX=[%f],mapTable[%d].ValueY=[%f]\n", ret, mapTable[ret].ValueX, ret, mapTable[ret].ValueY));
			if (ret++ >= size )
				break;		
			token = strtok_r( NULL, split1, &savePtr);
		}
	}

	return ret;
}


int strindex(char *str,char *substr)
{
	int end,i,j;

	end = strlen(str) - strlen(substr); /* 计算结束位置 */
	if ( end > 0 ) /* 子字符串小于字符串 */
	{
		for ( i = 0; i <= end; i++ )
			/* 用循环比较 */
			for ( j = i; str[j] == substr[j-i]; j++ )
				if ( substr[j-i+1] == '\0' ) /* 子字符串字结束 */
					return i + 1; /* 找到了子字符串 */
	}
	return 0;
}

int define_sscanf(char *src, char *dst, char* outBuf)
{
	int ret = -1;
	int i=0;
	int total_len = strlen(src);
	const char *p = src;
	memset(outBuf, 0, sizeof(outBuf));

	int result = strindex(src, dst);
	if(result == 0)
	{
		return ret;
	}
	int skip_len = (result-1) + strlen(dst);

	p = p + skip_len;

	while(i < (total_len-skip_len))
	{
		if(p[i] == ',' || p[i] == '\0')
		{
			break;
		}
		outBuf[i] = p[i];
		i++;
	}

    ret = i;
	return ret;
}


void LabelIF_GetDeviceQueryPrivatePara(char* config_str, PDEVICE_INFO_POLL stDeviceInfoPoll)
{
	char temp[56] = {0};
	int getParaRet = -1;
	switch(atoi(stDeviceInfoPoll->Upper_Channel))
	{
		//232串口接口
	case INTERFACE_RS232:
	case INTERFACE_RS485_1:        //485 1号口        *****************************************
	case INTERFACE_RS485_2:        //485 2号口        *  1号口  *  3号口  *  5号口  *  7号口  *
		//                 *****************************************
	case INTERFACE_RS485_3:       //485 3号口        *  2号口  *  4号口  *  6号口  *  8号口  *
	case INTERFACE_RS485_4:       //485 4号口        *****************************************
	case INTERFACE_RS485_5:       //485 5号口     485(1) ->ttyS3    485(8) ->SPI1.1 (serial0)   485(7) ->SPI1.2 (serial0)
	case INTERFACE_RS485_6:       //485 6号口     485(2) ->ttyS2    485(4) ->SPI1.1 (serial1)   485(3) ->SPI1.2 (serial1)
	case INTERFACE_RS485_7:       //485 7号口                       485(6) ->SPI1.1 (serial2)   485(5) ->SPI1.2 (serial2)
	case INTERFACE_RS485_8:       //485 8号口
		{
			//私有属性有 波特率、数据位、停止位、校验位
			stDeviceInfoPoll->BaudRate = 2400;
			getParaRet = define_sscanf(config_str, (char*)"bpt=",temp);      //取得设备波特率信息
			if (0 < getParaRet)
			{
				stDeviceInfoPoll->BaudRate = atoi(trim(temp, strlen(temp)));
				if (0 != stDeviceInfoPoll->BaudRate%600)
				{
					stDeviceInfoPoll->BaudRate = 2400;
				}
			}

			memset(temp, 0, sizeof(temp));
			stDeviceInfoPoll->databyte = 8;
			getParaRet = define_sscanf(config_str, (char*)"databyte=",temp);       //数据位
			if (0 < getParaRet)
			{
				if( 4 >= (stDeviceInfoPoll->databyte = atoi(temp)) )
				{
					stDeviceInfoPoll->databyte = 8;
				}
			}

			memset(temp, 0, sizeof(temp));
			stDeviceInfoPoll->stopbyte = 1;
			getParaRet = define_sscanf(config_str, (char*)"stopbyte=",temp);       //停止位
			if (0 < getParaRet)
			{
				if( 0 >= (stDeviceInfoPoll->stopbyte = atoi(temp)) )
				{
					stDeviceInfoPoll->stopbyte = 1;
				}
			}

			memset(temp, 0, sizeof(temp));
			sprintf( stDeviceInfoPoll->checkMode, "None" );
			getParaRet = define_sscanf(config_str, (char*)"checkbyte=", stDeviceInfoPoll->checkMode);
			if( getParaRet <= 0 || 0 >= strlen(stDeviceInfoPoll->checkMode))  //校验位
			{
				sprintf( stDeviceInfoPoll->checkMode, "None" );
			}

			stDeviceInfoPoll->timeOut = 500;
			getParaRet = define_sscanf(config_str, (char*)"rolltimeout=",temp);      //取得设备波特率信息
			if (0 < getParaRet)
			{
				stDeviceInfoPoll->timeOut = atoi(trim(temp, strlen(temp)));
				if (200 > stDeviceInfoPoll->timeOut)
				{
					stDeviceInfoPoll->timeOut = 500;
				}
			}
		}
		break;
		default:
			if (INTERFACE_NET_INPUT <= atoi(stDeviceInfoPoll->Upper_Channel))
			{
				//私有属性有 设备序列号、登录密码
			}
		break;
	}
}

//设备属性私有参数
void LabelIF_GetDeviceAttrQueryPrivatePara(char* config_str, PDEVICE_INFO_POLL stDeviceInfoPoll)
{
	char temp[256] = {0};
	stDeviceInfoPoll->Frequence = 1000*1000*60;
	if (0 < define_sscanf(config_str, (char*)"interval=",temp)) //获取采样频率
	{
		if( 0>= (stDeviceInfoPoll->Frequence = atoi(temp)))
		{
			stDeviceInfoPoll->Frequence = 1000*1000*60;
		}
	}
	memset(temp, 0, sizeof(temp));
	stDeviceInfoPoll->isUpload = 1;
	if (0 < define_sscanf(config_str, (char*)"isUpload=",temp))//获取上传标记频率
	{
		stDeviceInfoPoll->isUpload = (0 == atoi(temp)?0:1);
	}
}


void LabelIF_GetDeviceActionPrivatePara(char* config_str, PDEVICE_INFO_ACT stDeviceInfoPoll)
{
	char temp[56] = {0};
	int getParaRet = -1;
	switch(atoi(stDeviceInfoPoll->Upper_Channel))
	{
		//232串口接口
	case INTERFACE_RS232:
	case INTERFACE_RS485_1:        //485 1号口        *****************************************
	case INTERFACE_RS485_2:        //485 2号口        *  1号口  *  3号口  *  5号口  *  7号口  *
		//                 *****************************************
	case INTERFACE_RS485_3:       //485 3号口        *  2号口  *  4号口  *  6号口  *  8号口  *
	case INTERFACE_RS485_4:       //485 4号口        *****************************************
	case INTERFACE_RS485_5:       //485 5号口     485(1) ->ttyS3    485(8) ->SPI1.1 (serial0)   485(7) ->SPI1.2 (serial0)
	case INTERFACE_RS485_6:       //485 6号口     485(2) ->ttyS2    485(4) ->SPI1.1 (serial1)   485(3) ->SPI1.2 (serial1)
	case INTERFACE_RS485_7:       //485 7号口                       485(6) ->SPI1.1 (serial2)   485(5) ->SPI1.2 (serial2)
	case INTERFACE_RS485_8:       //485 8号口
		{

			//私有属性有 波特率、数据位、停止位、校验位
			stDeviceInfoPoll->BaudRate = 2400;
			getParaRet = define_sscanf(config_str, (char*)"bpt=",temp);      //取得设备波特率信息
			if (0 < getParaRet)
			{
				stDeviceInfoPoll->BaudRate = atoi(trim(temp, strlen(temp)));
				if (0 != stDeviceInfoPoll->BaudRate%600)
				{
					stDeviceInfoPoll->BaudRate = 2400;
				}
			}

			memset(temp, 0, sizeof(temp));
			stDeviceInfoPoll->databyte = 8;
			getParaRet = define_sscanf(config_str, (char*)"databyte=",temp);       //数据位
			if (0 < getParaRet)
			{
				if( 4 >= (stDeviceInfoPoll->databyte = atoi(temp)) )
				{
					stDeviceInfoPoll->databyte = 8;
				}
			}

			memset(temp, 0, sizeof(temp));
			stDeviceInfoPoll->stopbyte = 1;
			getParaRet = define_sscanf(config_str, (char*)"stopbyte=",temp);       //停止位
			if (0 < getParaRet)
			{
				if( 0 >= (stDeviceInfoPoll->stopbyte = atoi(temp)) )
				{
					stDeviceInfoPoll->stopbyte = 1;
				}
			}

			memset(temp, 0, sizeof(temp));
			sprintf( stDeviceInfoPoll->checkMode, "None" );
			getParaRet = define_sscanf(config_str, (char*)"checkbyte=", stDeviceInfoPoll->checkMode);
			if( getParaRet <= 0 || 0 >= strlen(stDeviceInfoPoll->checkMode))  //校验位
			{
				sprintf( stDeviceInfoPoll->checkMode, "None" );
			}

			stDeviceInfoPoll->timeOut = 500;
			getParaRet = define_sscanf(config_str, (char*)"rolltimeout=",temp);      //取得设备波特率信息
			if (0 < getParaRet)
			{
				stDeviceInfoPoll->timeOut = atoi(trim(temp, strlen(temp)));
				if (200 > stDeviceInfoPoll->timeOut)
				{
					stDeviceInfoPoll->timeOut = 500;
				}
			}
		}
		break;
	default:
		if (INTERFACE_NET_INPUT <= atoi(stDeviceInfoPoll->Upper_Channel))
		{
			//私有属性有 设备序列号、登录密码
		}
		break;
	}
}

//获取布防时间配置
void LabelIF_GetDefenceDate(char *strTimeSet, DEVICE_INFO_POLL *hash_node)
{
	char *split_result = NULL;
	char timeTrans[3] = {0};
	int startHour = 0;
	int startMinute = 0;
	int endHour = 0;
	int endMinute = 0;
	hash_node->startTime.tm_hour = 0;
	hash_node->startTime.tm_min = 0;
	hash_node->endTime.tm_hour = 24;
	hash_node->endTime.tm_min = 0;
	char* savePtr = NULL;
	split_result = strtok_r(strTimeSet, ";", &savePtr);
	if( split_result != NULL )
	{
		timeTrans[0] = split_result[0];
		timeTrans[1] = split_result[1];
		startHour = atoi(timeTrans);
		timeTrans[0] = split_result[2];
		timeTrans[1] = split_result[3];
		startMinute = atoi(timeTrans);

		split_result = strtok_r(NULL, ";", &savePtr);
		if(split_result == NULL)
		{
			return;
		}

		timeTrans[0] = split_result[0];
		timeTrans[1] = split_result[1];
		endHour = atoi(timeTrans);
		timeTrans[0] = split_result[2];
		timeTrans[1] = split_result[3];
		endMinute = atoi(timeTrans);

		hash_node->startTime.tm_hour = startHour;
		hash_node->startTime.tm_min = startMinute;
		hash_node->endTime.tm_hour = endHour;
		hash_node->endTime.tm_min = endMinute;
	}
}

//--------------------------------------

int LabelIF_DecodeActionResponse(PDEVICE_INFO_ACT device_node, unsigned char* buf, int bufLen)
{//lhy
	int ret = SYS_STATUS_FAILED;  
	if( strlen(device_node->Wck_Format) < 5 )
	{
		//DBG_LB(("RckFormat NULL\n"));
		return SYS_STATUS_SUBMIT_SUCCESS;
	}
	if(!LabelIF_CheckRecvFormat(device_node->Wck_Format, buf, bufLen))
		return SYS_STATUS_ACTION_RESP_FORMAT_ERROR;
	
	char szRespValue[128] = {0};
	if(SYS_STATUS_SUCCESS == DecodeByFormatEx(device_node->WD_Format, device_node->WD_Comparison_table, device_node->Id, buf, bufLen, szRespValue))
	{
		ret = atoi(szRespValue);
	}
	return ret;
}

bool LabelIF_CheckRecvFormat(char* checkFormat, unsigned char* buf, int bufLen)
{
    bool ret = false;
    FORMAT_CHECK format_check;
    char checkData[10] = {0};
    char checkCalResult[10] = {0};
    unsigned char tempSwap = 0;
	if (bufLen < 0 || bufLen > MAX_PACKAGE_LEN_TERMINAL)
	{
		DBG(("BufLen[%d] error\n", bufLen));
		return false;
	}
    if( strlen(checkFormat) < 5 )
    {
		//DBG_LB(("RckFormat NULL\n"));
		DBG(("checkFormatLen[%zu] \n", strlen(checkFormat)));
        return true;
    }
    //DBG_LB(("Rck check Begin\n"));
    GetCheckValue(",", checkFormat, &format_check);

    if( format_check.DS > bufLen || format_check.CS + format_check.CL > bufLen || format_check.CL < 1)
    {
		DBG_LB(("LabelIF_CheckRecvFormat Error DS[%d] CS[%d] CL[%d]\n", format_check.DS, format_check.CS, format_check.CL));
        return ret;
    }
    DBG_LB(("DS:[%d]  DE:[%d]  CS:[%d]  CE:[%d]  CL:[%d]\n", format_check.DS, format_check.DE, format_check.CS, format_check.CE, format_check.CL));
    if( format_check.CE == -1 )
    {
        memcpy(checkData, buf + format_check.CS, format_check.CL);
    }
    else
    {
        memcpy(checkData, buf + bufLen-format_check.CE, format_check.CL);
	}
	DBG_LB(("recv check:[%02x][%02x]\n", checkData[0], checkData[1]));
    if( format_check.C == 1 )    //高低字节转换
    {
        for( int j=0; j<format_check.CL; j++ )
        {
            tempSwap = checkData[j];
            checkData[j] = checkData[format_check.CL - 1 - j];
            checkData[format_check.CL - 1 - j] = tempSwap;
        }
    }
    switch(format_check.T)
	{
	case 1://crc16
		{
			unsigned short usCrcData = GetCrc16(buf+format_check.DS, bufLen-format_check.DS-format_check.DE);
			DBG_LB(("\n calc CRC check:"));
           // AddMsg((BYTE*)&usCrcData, 2);
			if( memcmp((BYTE*)&usCrcData, checkData, format_check.CL) == 0 )
            {
                ret = true;
            }
		}
		break;
	case 2://LRC8
        {
            unsigned char usLrcData = 0;
            //DBG_LB(("len:[%d],[%x][%x]\n", len, src_lrc[0], src_lrc[1]));
            for(int i=0; i<bufLen-format_check.DS-format_check.DE; i++)
            {
                usLrcData ^= buf[format_check.DS+i];
            }
            DBG_LB(("\n calc LRC check:[%02x]\n", usLrcData));
            checkCalResult[0] = usLrcData;
            if( checkCalResult[0] == checkData[0] )
            {
                ret = true;
            }
        }
		break;
    case 3:    //累加和低位0差
        {
		    ret = true;
        }
        break;
    case 4:    //二进制算术和
        {
            unsigned char usIncData_256 = 0;
            //DBG_LB(("len:[%d],[%x][%x]\n", len, src_inc_256[0], src_inc_256[1]));
            for(int i=0; i<bufLen-format_check.DS-format_check.DE; i++)
            {
                usIncData_256 += buf[format_check.DS+i];
            }
            DBG_LB(("\n calc 4 check:[%02x]\n", usIncData_256));
            checkCalResult[0] = usIncData_256;
            if( checkCalResult[0] == checkData[0] )
            {
                ret = true;
            }
        }
        break;
    case 5:    //算术和除以128
        {
            unsigned char usIncData_128 = 0;
            unsigned short incTemp = 0;
            for(int i=0; i<bufLen-format_check.DS-format_check.DE; i++)
            {
                incTemp += buf[format_check.DS+i];
                incTemp %= 128;
            }
            usIncData_128 = (unsigned char)incTemp;
            DBG_LB(("\n calc 5 check:[%02x]\n", usIncData_128));
            checkCalResult[0] = usIncData_128;
            if( checkCalResult[0] == checkData[0] )
            {
                ret = true;
            }
        }
        break;
	case 6:    //字节累加和，取反+1
		{
			unsigned char usIncData_128 = 0;
			unsigned char tmp=0;
			unsigned char* str = &buf[format_check.DS];
			for(int i=0;i<bufLen-format_check.DS-format_check.DE;i++)
			{
				tmp=*str+tmp;
				str++;
			}
			
			usIncData_128 = (255-tmp)+1;
			tmp = 0;
			DBG_LB(("BCD_2 [%02x] checkData[%s]\n", usIncData_128, checkData));
			if(1 > String2Hex(&tmp, (unsigned char*)checkData, 2))
				break;
			if (usIncData_128 == tmp)
			{
				ret = true;
			}
			DBG_LB(("\n calc 5 check:[%02x] [%s]\n", usIncData_128, (char*)checkData));
		}
		break;
	}
    return ret;
}


int LabelIF_EncodeFormat(char* format, char* self_id, char* szCmd, char* pInData, unsigned char* pOutBuf, int nBufSize)
{
	//Self_Id 编码
	int ret = -1;

	char szBufId[MAX_PACKAGE_LEN_TERMINAL*3] = {0};
	char szBufCmd[MAX_PACKAGE_LEN_TERMINAL*3] = {0};
	char szBufData[MAX_PACKAGE_LEN_TERMINAL*3] = {0};
	char szBufCheck[MAX_PACKAGE_LEN_TERMINAL*3] = {0};

	//插入设备ID号
	if (MAX_PACKAGE_LEN_TERMINAL*2 < strlen(format))
	{
		DBG(("EncodeFormat ERROR 1\n"));
		return -1;
	}
	memcpy(szBufId, format, strlen(format));
	if (0 > (ret = FormatID(format, self_id, szBufId, sizeof(szBufId))))
	{
		DBG(("EncodeFormat ERROR 2\n"));
		return -2;
	}
	DBG_LB(("FormatID[%s]\n",szBufId ));

	//插入CMD
	if (MAX_PACKAGE_LEN_TERMINAL*2 < strlen(szBufId))
	{
		DBG(("EncodeFormat ERROR 3\n"));
		return -3;
	}
	memcpy(szBufCmd, szBufId, strlen(szBufId));
	if (0 > (ret = FormatCMD(szBufId, szCmd, szBufCmd,  sizeof(szBufCmd))))
	{
		DBG(("EncodeFormat ERROR 4\n"));
		return -4;
	}
	DBG_LB(("FormatCMD[%s]\n",szBufCmd ));

	//插入数据
	if (MAX_PACKAGE_LEN_TERMINAL*2 < strlen(szBufCmd))
	{
		DBG(("EncodeFormat ERROR 5\n"));
		return -5;
	}
	memcpy(szBufData, szBufCmd, strlen(szBufCmd));
	if (0 > (ret = FormatData(szBufCmd, pInData, szBufData,  sizeof(szBufData))))
	{
		DBG(("EncodeFormat ERROR 6\n"));
		return -6;
	}
	DBG_LB(("FormatData[%s]\n",szBufData ));

	//插入校验码
	if (MAX_PACKAGE_LEN_TERMINAL*2 < strlen(szBufData))
	{
		DBG(("EncodeFormat ERROR 7\n"));
		return -7;
	}
	strcpy(szBufCheck, szBufData);
	if(0 > (ret = FormatCheck(szBufData, szBufCheck, sizeof(szBufCheck))))
	{
		DBG(("EncodeFormat ERROR 8\n"));
		return -8;
	}
	DBG_LB(("FormatCheck[%s]\n",szBufCheck ));

	//输出数据
	memset(pOutBuf, 0, nBufSize);
	ret = String2Hex(pOutBuf, (unsigned char*)szBufCheck, strlen(szBufCheck));
	if (MAX_PACKAGE_LEN_TERMINAL < ret)
	{
		return -9;
	}
	return ret;
}

int FormatID(char* format, char* id, char* outBuf, int nBufSize)
{	

	char szPara[MAX_PACKAGE_LEN_TERMINAL] = {0};	

	FORMAT_PARA formatPara;


	int lableLen = 0;
	char* lablePos = GetLable(format, (char*)"id", lableLen, szPara);
	if (NULL == lablePos)
	{
	//	DBG_LB(("Can not find Lable id\n"));
		return 0;
	}

	GetValue(",", szPara,  &formatPara);

	char self_id[125] = {0};
	char per = '%';
	char self_id_format[56] = {0};
    char temp[2] = {0};
    if( BCD == formatPara.DataType)          //ID为BCD码
    {
		if (formatPara.DataLen*2-strlen(id) < 0 )
		{
			return -1;
		}
		else if( formatPara.DataLen*2-strlen(id) == 0 )
        {
            sprintf(self_id, "%s", id);
        }
        else
        {
            sprintf(self_id_format, "%c0*d%cs", per, per);
            sprintf(self_id, self_id_format, formatPara.DataLen*2-strlen(id), 0, id);
        }
        //DBG_LB(("self_id[%s]\n",self_id));
        if (1 == formatPara.ChangeBytes)
        {
            for(int i=0; i<formatPara.DataLen; i+=2)
            {
                memcpy( temp, &self_id[i], 2);
                memcpy( &self_id[i], &self_id[formatPara.DataLen*2-i-2], 2);
                memcpy( &self_id[formatPara.DataLen*2-i-2], temp, 2);
                //DBG_LB(("self_id[%s]\n",self_id));
            }
        }
    }
	else if (BCD_2 == formatPara.DataType)//两层数字符串转字节码
	{
		switch (formatPara.RST)
		{
		case BCD_2:
			{

				if (formatPara.DataLen-strlen(id) < 0 )
				{
					return -1;
				}
				else if( formatPara.DataLen-strlen(id) == 0 )
				{
					sprintf(self_id, "%s", id);
				}
				else
				{
					sprintf(self_id_format, "%c0%ds", per, formatPara.DataLen-(TKU32)strlen(id));
					sprintf(self_id, self_id_format,  id);
				}
			}
			break;
		case CHARS:
		case BYTES:
			if (56 < strlen(id) || 0 == Hex2String(self_id, id, strlen(id)*2))
			{
				return -1;
			}
			DBG_LB(("BCD_2 222 SelfId[%s]\n", self_id));
			break;
		default:
			{
				unsigned int unSelfId = atoi(id);
				char tempSelf[128] = {0};
				sprintf(self_id_format, "%c0%dX", per, formatPara.DataLen*2);
				if (1 == formatPara.ChangeBytes)
				{
					//高低字节转换，lhy
					switch (formatPara.DataLen)
					{
					case 2:
						unSelfId = ConvertUShort((unsigned short)unSelfId);
						break;
					case 4:
						unSelfId = TurnDWord(unSelfId);
						break;
					}
				}
				sprintf(tempSelf, self_id_format, unSelfId);

				DBG_LB(("BCD_2 default format[%s] tempSelf[%s], self_id_format[%s] [%d]\n", format, tempSelf, self_id_format, unSelfId));
				if (128 < strlen(tempSelf) || 0 == Hex2String(self_id, tempSelf, strlen(tempSelf)*2))
				{
					return -1;
				}
			}
			break;
		}
		DBG_LB(("BCD_2 SelfId[%s]\n", self_id));
	}
	else if (CHARS == formatPara.DataType)//两层数字符串转字节码
	{
		if (56 < strlen(id) || 0 == Hex2String(self_id, id, strlen(id)*2))
		{
			return -1;
		}			
		DBG_LB(("CHARS SelfId[%s]\n", self_id));
	}
    else           //字节码
    {
	    sprintf(self_id_format, "%c0%dx", per, formatPara.DataLen*2);
	    unsigned int unSelfId = atoi(id);

        if (1 == formatPara.ChangeBytes)
        {
		    //高低字节转换，lhy
		    switch (formatPara.DataLen)
            {
		    case 2:
			    unSelfId = ConvertUShort((unsigned short)unSelfId);
			    break;
		    case 4:
			    unSelfId = TurnDWord(unSelfId);
			    break;
            }
        }

	    sprintf(self_id, self_id_format, unSelfId);
    }
	

    //DBG_LB(("self_id[%s]\n",self_id));
	memset(outBuf, 0, nBufSize);
	if (lablePos - format > 0)
	{
		strncpy(outBuf, format, lablePos - format);
	}

	strcat(outBuf, self_id);
   // DBG_LB(("outBuf1111[%s]\n",outBuf));
	strcat(outBuf, lablePos + lableLen);
    //DBG_LB(("outBuf2222[%s]\n",outBuf));

	return 1;
}

int FormatCMD(char* format, char* szCmd, char* outBuf, int nBufSize)
{
	char szPara[MAX_PACKAGE_LEN_TERMINAL] = {0};	

	//FORMAT_PARA formatPara;

	int lableLen = 0;
	char* lablePos = GetLable(format, (char*)"cmd", lableLen, szPara);
	if (NULL == lablePos)
	{
		return 0;
	}

	//GetValue(",", szPara,  &formatPara);

	char tempBuf[MAX_PACKAGE_LEN_TERMINAL] = {0};
	strcpy(tempBuf, szCmd);

	memset(outBuf, 0, nBufSize);
	//DBG_LB(("outBuf:--------[%s]\n", outBuf));
	if (lablePos - format > 0)
	{
		strncpy(outBuf, format, lablePos - format);
	}
	DBG_LB(("format:[%s]\n", format));
	DBG_LB(("lablePos:[%s]\n", lablePos));
	DBG_LB(("outbuf:[%s]\n", outBuf));
    DBG_LB(("tempBuf:[%s]\n", tempBuf));

	if (0 < strlen(tempBuf))
	{
		strcat(outBuf, tempBuf);
	}
	strcat(outBuf, lablePos + lableLen);
    DBG_LB(("outbufcmd:[%s]\n", outBuf));
	return 1;
}

int FormatData(char* format, char* data, char* outBuf, int nBufSize)
{
	char szPara[MAX_PACKAGE_LEN_TERMINAL] = {0};

	FORMAT_PARA formatPara;

	char temp_format[512] = {0};

	int lableLen = 0;
	char* lablePos = GetLable(format, (char*)"data", lableLen, szPara);
	if (NULL == lablePos)
	{
//		DBG_LB(("Can not find lable Data\n"));
		DBG(("FormatData 0\n"));
		return 0;
	}
	if (0 >= strlen(data))
	{
		DBG(("FormatData 1\n"));
		return -1;
	}

	GetValue(",", szPara,  &formatPara);

	char tempBuf[MAX_PACKAGE_LEN_TERMINAL] = {0};
	if (CHARS == formatPara.DataType)//如果是普通字符串，则要转换成16进制字符串，lhy
	{
		if(256 < strlen(data) || 0 == Hex2String(tempBuf, data, strlen(data)*2))
		{
			DBG(("FormatData 2\n"));
			return -1;
		}
	}
	else if (BCD_2 == formatPara.DataType)
	{
		switch (formatPara.RST)
		{
		case BCD_2:
			{
				if(256 < strlen(data)  || 0 == Hex2String(tempBuf, data, strlen(data)*2))
				{
					DBG(("FormatData 3\n"));
					return -1;
				}
			}
			break;
		case CHARS:
			if (256 < strlen(data) || 0 == Hex2String(tempBuf, data, strlen(data)*2))
			{
				DBG(("FormatData 4\n"));
				return -1;
			}
			break;
		default:
			{
				unsigned int unSelfId = atoi(data);
				char tempSelf[128] = {0};
				char per = '%';
				sprintf(temp_format, "%c0%dX", per, formatPara.DataLen*2);
				if (1 == formatPara.ChangeBytes)
				{
					//高低字节转换，lhy
					switch (formatPara.DataLen)
					{
					case 2:
						unSelfId = ConvertUShort((unsigned short)unSelfId);
						break;
					case 4:
						unSelfId = TurnDWord(unSelfId);
						break;
					}
				}
				sprintf(tempSelf, temp_format, unSelfId);

				if (128 < strlen(tempSelf) || 0 == Hex2String(tempBuf, tempSelf, strlen(tempSelf)*2))
				{
					DBG(("FormatData 5\n"));
					return -1;
				}
			}
			break;
		}
		DBG_LB(("BCD_2 FormatData[%s]\n", tempBuf));
	}
	else if(UCHAR == formatPara.DataType)
	{
		char data_format[256] = {0};
		char per = '%';
		sprintf(data_format, "%c0%dx", per, formatPara.DataLen*2);
	    unsigned int undata = atoi(data);

        if (1 == formatPara.ChangeBytes)
        {
		    //高低字节转换，lhy
		    switch (formatPara.DataLen)
            {
		    case 2:
			    undata = ConvertUShort((unsigned short)undata);
			    break;
		    case 4:
			    undata = TurnDWord(undata);
			    break;
            }
        }
	    sprintf(tempBuf, data_format, undata);
	}
	/*   瓦尔通信短信猫   冒号之前string  冒号之后unicode     lsd  20160802 */
	/* S  M  S  U  1  3  7  5  7  1  1  1  2  4  1  ,  0  4  :  中    国    */
	/* 53 4D 53 55 31 33 37 35 37 31 31 31 32 34 31 2C 30 34 3A 4E 2D 56 fd */
	else if (11 == formatPara.DataType)
	{
		int telLen = 0;
		char szTel[15] = {0};
		GetLable(data, (char*)"tel", telLen, szTel);
//		DBG(("szTel[%s]\n", szTel));

		int txtLen = 0;
		char szTxt[141] = {0};
		GetLable(data, (char*)"txt", txtLen, szTxt);
//		DBG(("szTxt[%s]\n", szTxt));

		unsigned char szUniCode[141] = {0};
		txtLen = GB2UniCode(szTxt, (unsigned char*)szUniCode);
//		DBG(("szUniCode[%s]\n", szUniCode));

		char szTmp[200] = {0};
		sprintf(szTmp, "SMSU%s,%d:", szTel, txtLen);
//		DBG(("szTmp[%s]\n", szTmp));
		Hex2String(tempBuf, szTmp, strlen(szTmp)*2);
//		DBG(("tempBuf[%s]\n", tempBuf));
		
		for(int i=0;i<txtLen;i++ ) 
		{
			char tmpTxt[3] = {0};
			sprintf(tmpTxt, "%02X",szUniCode[i]);
			strcat(tempBuf, tmpTxt);
		}
	}
	else
	{
		strcpy(tempBuf, data);
	}

	memset(outBuf, 0, nBufSize);   
	strncpy(outBuf, format, lablePos - format);

	if (0 < strlen(tempBuf))
	{
		strcat(outBuf, tempBuf);
	}
	strcat(outBuf, lablePos + lableLen);

	return 1;
}

int FormatCheck(char* format, char* outBuf, int nBufSize)
{	
	int ret = -1;
	char szPara[MAX_PACKAGE_LEN_TERMINAL] = {0};
	FORMAT_PARA formatPara;

	int lableLen = 0;
	char* lablePos = GetLable(format, (char*)"check", lableLen, szPara);
	if (NULL == lablePos)
	{
	//	DBG_LB(("Can not find lable check\n"));
		return 0;
	}

	GetValue(",", szPara,  &formatPara);

	char szCheckValue[128] = {0};
	int nCheckLen = 0;
    if(formatPara.EndBytes < 0)
        return 0;

    nCheckLen = strlen(format) - formatPara.EndBytes - formatPara.StartBytes - lableLen;
	if (nCheckLen < 0)
	{
		return -1;
	}
    if((lablePos - format)>formatPara.StartBytes)          //检验起始地址在check=前面
		memcpy(szCheckValue, format + formatPara.StartBytes, nCheckLen);
    else                                          //检验起始地址在check=后面
		memcpy(szCheckValue, format + formatPara.StartBytes + lableLen, nCheckLen);
	
	//DBG_LB(("checkLen[%d] value[%s] lablelen[%d]\n", nCheckLen, szCheckValue, lableLen));
    char per = '%';
	switch(formatPara.DataType)
	{
	case 1://crc16
		{
			//字符串转换为字节
			unsigned char src[MAX_PACKAGE_LEN_TERMINAL] = {0};
			int len = String2Hex(src, (unsigned char*)szCheckValue, nCheckLen);
			unsigned short usCrcData = GetCrc16(src, len);
			
			if (1 == formatPara.ChangeBytes)
			{
				//高低字节互换lhy
				usCrcData = ConvertUShort(usCrcData);
			}
			
			
			char szCrc[56] = {0};
			char crc_format[56] = {0};

			sprintf(crc_format, "%c0%dx", per, formatPara.DataLen*2);
			sprintf(szCrc, crc_format, usCrcData);

			memset(outBuf, 0, nBufSize);

			if (lablePos - format > 0)
			{
				strncpy(outBuf, format, lablePos - format);
				outBuf[lablePos - format] = '\0';
			}
			strcat(outBuf, szCrc);
			int tempLen = lablePos - format + lableLen; 
			if ( tempLen < (int)strlen(format))
			{
				strcat(outBuf, lablePos + lableLen);
			}
			ret = 1;
		}
		break;
	case 2://LRC8
        {
            unsigned char src_lrc[MAX_PACKAGE_LEN_TERMINAL] = {0};
			int len = String2Hex(src_lrc, (unsigned char*)szCheckValue, nCheckLen);
            unsigned char usLrcData = 0;
            //DBG_LB(("len:[%d],[%x][%x]\n", len, src_lrc[0], src_lrc[1]));
            for(int i=0; i<len; i++)
            {
                usLrcData ^= src_lrc[i];
            }
            if (1 == formatPara.ChangeBytes)
			{
				//高低字节互换lhy
				usLrcData = ConvertUShort(usLrcData);
			}

            char szLrc[56] = {0};
			char lrc_format[56] = {0};

			sprintf(lrc_format, "%c0%dx", per, formatPara.DataLen*2);
			sprintf(szLrc, lrc_format, usLrcData);

			memset(outBuf, 0, nBufSize);

			if (lablePos - format > 0)
			{
				strncpy(outBuf, format, lablePos - format);
				outBuf[lablePos - format] = '\0';
			}
			strcat(outBuf, szLrc);
			int tempLen = lablePos - format + lableLen; 
			if ( tempLen < (int)strlen(format))
			{
				strcat(outBuf, lablePos + lableLen);
			}
		    ret = 1;
        }
		break;
    case 3:    //累加和低位0差
        {
            unsigned char src_inc[MAX_PACKAGE_LEN_TERMINAL] = {0};
			int len = String2Hex(src_inc, (unsigned char*)szCheckValue, nCheckLen);
            unsigned char usIncData = 0;
            //DBG_LB(("len:[%d],[%x][%x]\n", len, src_lrc[0], src_lrc[1]));
            for(int i=0; i<len; i++)
            {
                usIncData += src_inc[i];
            }
            usIncData = 0x00 - usIncData;

            char szInc[56] = {0};
			char inc_format[56] = {0};

			sprintf(inc_format, "%c0%dx", per, formatPara.DataLen*2);
			sprintf(szInc, inc_format, usIncData);

			memset(outBuf, 0, nBufSize);

			if (lablePos - format > 0)
			{
				strncpy(outBuf, format, lablePos - format);
				outBuf[lablePos - format] = '\0';
			}
			strcat(outBuf, szInc);
			int tempLen = lablePos - format + lableLen; 
			if ( tempLen < (int)strlen(format))
			{
				strcat(outBuf, lablePos + lableLen);
			}
		    ret = 1;
        }
        break;
    case 4:    //二进制算术和
        {
            unsigned char src_inc_256[MAX_PACKAGE_LEN_TERMINAL] = {0};
			int len = String2Hex(src_inc_256, (unsigned char*)szCheckValue, nCheckLen);
            unsigned char usIncData_256 = 0;
            //DBG_LB(("len:[%d],[%x][%x]\n", len, src_inc_256[0], src_inc_256[1]));
            for(int i=0; i<len; i++)
            {
                usIncData_256 += src_inc_256[i];
            }

            char szInc_256[56] = {0};
			char inc_format_256[56] = {0};

			sprintf(inc_format_256, "%c0%dx", per, formatPara.DataLen*2);
			sprintf(szInc_256, inc_format_256, usIncData_256);

			memset(outBuf, 0, nBufSize);

			if (lablePos - format > 0)
			{
				strncpy(outBuf, format, lablePos - format);
				outBuf[lablePos - format] = '\0';
			}
			strcat(outBuf, szInc_256);
			int tempLen = lablePos - format + lableLen; 
			if ( tempLen < (int)strlen(format))
			{
				strcat(outBuf, lablePos + lableLen);
			}
		    ret = 1;
        }
        break;
    case 5:        //算术和除以128的余数校验
        {
            unsigned char src_inc_128[MAX_PACKAGE_LEN_TERMINAL] = {0};
            int len = String2Hex(src_inc_128, (unsigned char*)szCheckValue, nCheckLen);
            unsigned char usIncData_128 = 0;
            unsigned short incTemp = 0;
            for(int i=0; i<len; i++)
            {
                incTemp += src_inc_128[i];
                incTemp %= 128; 
            }
            usIncData_128 = (unsigned char)incTemp;

            char szInc_128[56] = {0};
			char inc_format_128[56] = {0};

			sprintf(inc_format_128, "%c0%dx", per, formatPara.DataLen*2);
			sprintf(szInc_128, inc_format_128, usIncData_128);

			memset(outBuf, 0, nBufSize);

			if (lablePos - format > 0)
			{
				strncpy(outBuf, format, lablePos - format);
				outBuf[lablePos - format] = '\0';
			}
			strcat(outBuf, szInc_128);
			int tempLen = lablePos - format + lableLen; 
			if ( tempLen < (int)strlen(format))
			{
				strcat(outBuf, lablePos + lableLen);
			}
		    ret = 1;
        }
        break;
	case 6:        //字节累加和，取反+1
		{
			unsigned char src_inc_128[MAX_PACKAGE_LEN_TERMINAL] = {0};
			int len = String2Hex(src_inc_128, (unsigned char*)szCheckValue, nCheckLen);
			unsigned char usIncData_128 = 0;

			unsigned char tmp=0,j=0;
			unsigned char* str = src_inc_128;
			for(j=0;j<len;j++)
			{
				tmp=*str+tmp;
				str++;
			}
			usIncData_128 = (255-tmp)+1;

			char szInc_128[56] = {0};
			char szInc_Temp[56] = {0};
			char inc_format_128[56] = {0};

			sprintf(inc_format_128, "%c0%dX", per, formatPara.DataLen*2);
			sprintf(szInc_Temp, inc_format_128, usIncData_128);


			DBG_LB(("FormatCheck szCheckValue[%s] nCheckLen[%d]  inc_format_128[%s] szInc_Temp[%s] usIncData_128[%02x]\n", szCheckValue, nCheckLen,inc_format_128, szInc_Temp, usIncData_128));
			AddMsg(src_inc_128, len);
			if(1 != Hex2String(szInc_128, szInc_Temp, strlen(szInc_Temp)*2))
				return -1;

			DBG_LB(("FormatCheck szInc_128[%s] \n", szInc_128));
			memset(outBuf, 0, nBufSize);

			if (lablePos - format > 0)
			{
				strncpy(outBuf, format, lablePos - format);
				outBuf[lablePos - format] = '\0';
			}
			strcat(outBuf, szInc_128);
			int tempLen = lablePos - format + lableLen; 
			if ( tempLen < (int)strlen(format))
			{
				strcat(outBuf, lablePos + lableLen);
			}
			DBG_LB(("FormatCheck BCD2[%s]\n", szInc_128));
			ret = 1;
		}
		break;
		case 7:    //XOR
        {
            unsigned char src_inc_256[MAX_PACKAGE_LEN_TERMINAL] = {0};
			int len = String2Hex(src_inc_256, (unsigned char*)szCheckValue, nCheckLen);
            unsigned char usIncData_256 = 0;
            //DBG_LB(("len:[%d],[%x][%x]\n", len, src_inc_256[0], src_inc_256[1]));
            for(int i=0; i<len; i++)
            {
                usIncData_256 ^= src_inc_256[i];
            }

            char szInc_256[56] = {0};
			char inc_format_256[56] = {0};

			sprintf(inc_format_256, "%c0%dx", per, formatPara.DataLen*2);
			sprintf(szInc_256, inc_format_256, usIncData_256);

			memset(outBuf, 0, nBufSize);

			if (lablePos - format > 0)
			{
				strncpy(outBuf, format, lablePos - format);
				outBuf[lablePos - format] = '\0';
			}
			strcat(outBuf, szInc_256);
			int tempLen = lablePos - format + lableLen; 
			if ( tempLen < (int)strlen(format))
			{
				strcat(outBuf, lablePos + lableLen);
			}
		    ret = 1;
        }
        break;
	}
	return ret;
}


/******************************************************************************/
/* Function Name  : DecodeByFormatEx                                            */
/* Description    : 解码码收到串口的数据                                      */
/* Input          : BYTE	             通道号                               */
/*                  BYTE	             通信编号                             */
/*                  char* sendSrc        通信协议格式                         */
/*                  char* crc_control    检验格式                             */
/* Output         : BYTE* sendBuf        欲发送至串口的数据                   */
/* Return         : int sendLen          发送数据长度                         */
/******************************************************************************/
int DecodeByFormatEx(const char* D_Format, char* Comparison_table, char* devId,  unsigned char* pInBuf, int inLen, char* stOutValue)
{
	int ret = SYS_STATUS_ACTION_DECODE_ERROR;
	char szRD[MAX_PACKAGE_LEN_TERMINAL] = {0};

	FORMAT_TIME timeFormat;
	FORMAT_HEAD headFormat;
	FORMAT_PARA rdFormat;
	memset( (unsigned char*)&timeFormat, 0, sizeof(FORMAT_TIME) );
	memset( (unsigned char*)&headFormat, 0, sizeof(FORMAT_HEAD) );
	memset( (unsigned char*)&rdFormat, 0, sizeof(FORMAT_PARA) );

	if (0 >= inLen || 0 >= strlen(D_Format))//解析格式为空，退出
	{
		return SYS_STATUS_ACTION_DECODE_FORMAT_ERROR;
	}
	
	strcpy(szRD, D_Format);

	char szRdNode[128] = {0};          //一对大括号之间的内容

	if(0 >= sscanf(szRD, "%*[{]%[^}]", szRdNode))
		return SYS_STATUS_ACTION_DECODE_FORMAT_ERROR;

	//char* ptrRD = strstr((char*)szRD, szRdNode) + strlen(szRdNode) + 1;

	/**************************************数据本身处理**********************************/
	memset( (unsigned char*)&headFormat, 0, sizeof(FORMAT_HEAD));
	GetHeadValue(",", szRdNode, &headFormat);           //取头部设置
	if( headFormat.ST == -1 )      //码流不正常
	{
		return SYS_STATUS_ACTION_DECODE_FORMAT_ERROR;
	}

	char szRdMidNode[64] = {0};      //用于存放大括号中中括号内容
	char szCaculateBuf[64] = {0};      //存放最终计算用的表达式
	float midResult[10] = {0};			//如果为计算式，最多支持10个临时值
	int midResultCnt = 0;

	char* midNum = NULL;
	char* savePtr = NULL;

	int VType = 0;
	if (CHARS == headFormat.T || BYTES ==  headFormat.T)
	{
		VType =  headFormat.T;
	}
	else
	{
		VType = FLOAT;
	}

	char * ptrRdNode = szRdNode;
	while(NULL != ptrRdNode &&  0 < sscanf(ptrRdNode, "%*[^[][%[^]]", szRdMidNode) )
	{
		ptrRdNode = strstr(ptrRdNode, szRdMidNode) + strlen(szRdMidNode);
		midNum = NULL;
		savePtr = NULL;
		midResultCnt = 0;

		midNum = strtok_r( szRdMidNode, ":", &savePtr);
		if (NULL == midNum)
		{
			return SYS_STATUS_ACTION_DECODE_ERROR;
		}

		midResultCnt = atoi(midNum);

		if( midResultCnt == 0 )//判断是否为计算用表达式
		{
			strcpy(szCaculateBuf, savePtr);
			continue;
		}

		if( midResultCnt >= 10 )//如果临时值编号超过10个，退出
		{
			return SYS_STATUS_ACTION_DECODE_ERROR;
		}

		GetValue(",", savePtr, &rdFormat);           //获取数值设置

		DBG_LB(("inLen:[%d]\n", inLen));
		//V2.0.0.6 lhy
		if(1 == rdFormat.LenType)//如果长度是倒序
		{
			rdFormat.DataLen = (inLen - rdFormat.StartBytes - rdFormat.DataLen);
		}
		if( rdFormat.StartBytes + rdFormat.DataLen > inLen || rdFormat.StartBytes == -1 || rdFormat.DataLen == -1 || 0 ==  rdFormat.DataLen)
		{
			return SYS_STATUS_ACTION_DECODE_FORMAT_ERROR;
		}
		if( (rdFormat.RT == 2) && ( rdFormat.RL == -1 || rdFormat.R == -1 || rdFormat.R + rdFormat.RL > inLen ) )
		{
			return SYS_STATUS_ACTION_DECODE_FORMAT_ERROR;
		}


		BYTE tempdata[MAX_PACKAGE_LEN_TERMINAL] = {0};
		int tValue = 0;
		int tResult = 0;
		int tCnt = 0;
		float ratio = 0;
		BYTE tempSwap = 0;
		unsigned char tempBCD2[1024] = {0};

		if( rdFormat.DataLen > 0 )
		{
			memcpy(tempdata, pInBuf + rdFormat.StartBytes, rdFormat.DataLen);

			DBG_LB(("DecodeByFormat Temp Value\n"));
			AddMsg(tempdata, rdFormat.DataLen);

			if( rdFormat.ChangeBytes == 1 && STREAM_BCD_2 != headFormat.ST)    //高低字节转换
			{
				for( int j=0; j<rdFormat.DataLen/2; j++ )
				{
					tempSwap = tempdata[j];
					tempdata[j] = tempdata[rdFormat.DataLen - 1 - j];
					tempdata[rdFormat.DataLen - 1 - j] = tempSwap;
				}
			}
			if( headFormat.ST == STREAM_BYTES_33 )     //电能表字节流
			{
				for( int j=0; j<rdFormat.DataLen; j++ )
				{
					tempdata[j] -= 0x33;
				}
			}

			/************************************获取基础数据***********************************/
			switch(rdFormat.DataType)
			{
			case CHARS://字符串
			case BYTES://字节码
			case BCD:
				if(STREAM_CHARS == headFormat.ST || STREAM_BCD_2 == headFormat.ST)
					memcpy(stOutValue, tempdata, rdFormat.DataLen);
				else
					Hex2String( stOutValue, (char *)tempdata, rdFormat.DataLen*2);
				break;
			case SHORT://短整形
				if(STREAM_CHARS == headFormat.ST)    //字符流
					midResult[midResultCnt] = atoi((char*)tempdata)*1.0;
				else if(STREAM_BYTES == headFormat.ST)   //字节流
					midResult[midResultCnt] = ByteToShort(tempdata)*1.0;
				else if(STREAM_BCD_2 == headFormat.ST)	//BCD2
				{
					memset(tempBCD2, 0, sizeof(tempBCD2));
					if(1 > String2Hex(tempBCD2, tempdata, rdFormat.DataLen))
					{
						DBG_LB(("DecodeByFormatEx 111 String2Hex Failed\n"));
						return false;
					}
					AddMsg(tempBCD2, 2);
					midResult[midResultCnt] = *((short*)tempBCD2)*1.0;
				}
				else if(STREAM_BCD == headFormat.ST || STREAM_BYTES_33 == headFormat.ST)     //BCD流
				{
					for( tCnt=0; tCnt<rdFormat.DataLen; tCnt++ )
					{
						tResult *= 100;
						tValue = (tempdata[tCnt] >> 4) * 10 + ((BYTE)(tempdata[tCnt] << 4) >> 4);
						tResult += tValue;
					}
					midResult[midResultCnt] = tResult*1.0;
				}
				else
					midResult[midResultCnt] = ByteToShort(tempdata)*1.0;
				break;
			case INTEGER://整形
				if(STREAM_CHARS == headFormat.ST)
					midResult[midResultCnt] = atoi((char*)tempdata)*1.0;
				else if(STREAM_BYTES == headFormat.ST)
					midResult[midResultCnt] = *((int*)tempdata)*1.0;

				else if(STREAM_BCD_2 == headFormat.ST)
				{
					memset(tempBCD2, 0, sizeof(tempBCD2));
					if(1 > String2Hex(tempBCD2, tempdata, rdFormat.DataLen))
						return false;
					midResult[midResultCnt] = *((int*)tempBCD2)*1.0;
				}
				else if(STREAM_BCD == headFormat.ST || STREAM_BYTES_33 == headFormat.ST)       //BCD码  33流电能表专用，BCD计算
				{
					for( tCnt=0; tCnt<rdFormat.DataLen; tCnt++ )
					{
						tResult *= 100;
						tValue = (tempdata[tCnt] >> 4) * 10 + ((BYTE)(tempdata[tCnt] << 4) >> 4);
						tResult += tValue;
					}
					midResult[midResultCnt] = tResult*1.0;
				}
				else
					midResult[midResultCnt] = *((int*)tempdata)*1.0;
				break;
			case CHAR:    //字符
				if(STREAM_CHARS == headFormat.ST)
					midResult[midResultCnt] = atoi((char*)tempdata)*1.0;
				else if(STREAM_BYTES == headFormat.ST)
					midResult[midResultCnt] = *((char*)tempdata)*1.0;

				else if(STREAM_BCD_2 == headFormat.ST)
				{
					memset(tempBCD2, 0, sizeof(tempBCD2));
					if(1 > String2Hex(tempBCD2, tempdata, rdFormat.DataLen))
						return false;
					midResult[midResultCnt] = *((char*)tempBCD2)*1.0;
				}
				else if(STREAM_BCD == headFormat.ST)       //BCD码
				{
					for( tCnt=0; tCnt<rdFormat.DataLen; tCnt++ )
					{
						tResult *= 100;
						tValue = (tempdata[tCnt] >> 4) * 10 + ((BYTE)(tempdata[tCnt] << 4) >> 4);
						tResult += tValue;
					}
					midResult[midResultCnt] = *((char*)tResult)*1.0;
				}
				else
					midResult[midResultCnt] = *((char*)tempdata)*1.0;
				break;
			case UCHAR:    //字符
				if(STREAM_CHARS == headFormat.ST)
				{
					if (0 >= strlen(trim((char*)tempdata, strlen((char*)tempdata))))
						return false;
					midResult[midResultCnt] = atoi(trim((char*)tempdata, strlen((char*)tempdata)))*1.0;
				}
				else if(STREAM_BCD_2 == headFormat.ST)
				{
					memset(tempBCD2, 0, sizeof(tempBCD2));
					if(1 > String2Hex(tempBCD2, tempdata, rdFormat.DataLen))
						return false;
					midResult[midResultCnt] = *((unsigned char*)tempBCD2)*1.0;
				}
				else if(STREAM_BYTES == headFormat.ST)
					midResult[midResultCnt] = *((unsigned char*)tempdata)*1.0;
				else if(STREAM_BCD == headFormat.ST)       //BCD码
				{
					for( tCnt=0; tCnt<rdFormat.DataLen; tCnt++ )
					{
						tResult *= 100;
						tValue = (tempdata[tCnt] >> 4) * 10 + ((BYTE)(tempdata[tCnt] << 4) >> 4);
						tResult += tValue;
					}
					midResult[midResultCnt] = *((unsigned char*)tResult)*1.0;
				}
				else
					midResult[midResultCnt] = *((unsigned char*)tempdata)*1.0;
				break;
			case FLOAT:
			case DOUBLE:    //浮点型
				//ASCII
				if(STREAM_CHARS == headFormat.ST)
					midResult[midResultCnt] = atof((char*)tempdata)*1.0;
				//字节流
				else if(STREAM_BYTES == headFormat.ST || STREAM_BYTES_33 == headFormat.ST)
					midResult[midResultCnt] = *((float*)tempdata);
				//BCD码流
				else if(STREAM_BCD_2 == headFormat.ST)
				{
					memset(tempBCD2, 0, sizeof(tempBCD2));
					if(1 > String2Hex(tempBCD2, tempdata, rdFormat.DataLen))
					{
						DBG_LB(("STREAM_BCD_2 String2Hex Failed\n"));
						return false;
					}
					AddMsg(tempBCD2, 2);
					midResult[midResultCnt] = *((float*)tempBCD2);
				}
				else if(STREAM_BCD == headFormat.ST)       //BCD流
				{
					for( tCnt=0; tCnt<rdFormat.DataLen; tCnt++ )
					{
						tResult *= 100;
						tValue = (tempdata[tCnt] >> 4) * 10 + ((BYTE)(tempdata[tCnt] << 4) >> 4);
						tResult += tValue;
					}
					midResult[midResultCnt] = tResult*1.0;
				}
				else
				{
					midResult[midResultCnt] = *((float*)tempdata);
				}
				break;
			default:
				DBG(("DecodeByFormatEx SYS_STATUS_ACTION_RESP_FORMAT_ERROR datetype[%d]\n", rdFormat.DataType));
				return SYS_STATUS_ACTION_RESP_FORMAT_ERROR;
			}
		}

		DBG_LB(("----------------midResult[%f] [%d]\n",midResult[midResultCnt], rdFormat.RT));
		/***********************************获取系数值********************************************/
		BYTE table_type = 0;      //参数表种类
		int len = 0;
		char szTableList[1024] = {0};
		FORMAT_MAP_TABLE mapTable[200] = {{0}};
		if( rdFormat.RT == 0 )               //系数是一个常量
		{			
			DBG_LB(("系数是一个常量\n"));
			ratio = rdFormat.R;
		}
		else if( rdFormat.RT == 1 )          //系数需要常量查表
		{
			DBG_LB(("系数需要常量查表\n"));
			GetCalLable(Comparison_table, headFormat.DT, midResultCnt, len, szTableList, &table_type);
			if( table_type != 1 && table_type != 2 )
			{
				return SYS_STATUS_ACTION_COMPARE_FORMAT_ERROR;
			}
			int cnt = GenerateTable(";", "~", szTableList, mapTable, 200);
			GetValueByTable(1, rdFormat.R, table_type, NULL, &rdFormat, headFormat, mapTable, cnt, &ratio);
		}
		else if( rdFormat.RT == 2 )          //系数需要截取值查表
		{
			DBG_LB(("系数需要截取值查表\n"));
			GetCalLable(Comparison_table, headFormat.DT, midResultCnt, len, szTableList, &table_type);
			if( table_type != 1 && table_type != 2 )
			{
				return SYS_STATUS_ACTION_COMPARE_FORMAT_ERROR;
			}
			int cnt = GenerateTable(";", "~", szTableList, mapTable, 200);
			GetValueByTable(2, 0, table_type, pInBuf, &rdFormat, headFormat, mapTable, cnt, &ratio);
		}

		DBG_LB(("R result:[%f]\n", ratio));

		//确定实际值计算方法   D:直接截取实际值    R:直接用系数作为实际值    DR:截取值与系数相乘得实际值
		if( rdFormat.DC[0] == 'D' &&  rdFormat.DC[1] == 0 )
		{
		}
		else if( rdFormat.DC[0] == 'R' )
		{
			midResult[midResultCnt] = ratio;
		}
		else if( rdFormat.DC[0] == 'D' && rdFormat.DC[1] == 'R' )
		{
			midResult[midResultCnt] *= ratio;
		}
		else
		{
			return SYS_STATUS_ACTION_DECODE_FORMAT_ERROR;
		}
	}

	/**********************************正则表达式计算***************************************/
	if( VType == FLOAT )
	{
		char tempCalFunc[128] = {0};
		float result = 0;
		//DBG_LB(("逆波兰式:%s\n", szCaculateBuf));
		transPolish(szCaculateBuf, tempCalFunc);
		//DBG_LB(("逆波兰式转换后:%s\n", tempCalFunc));
		result = compvalue(tempCalFunc, midResult);
		sprintf(stOutValue, "%.2f", result);
		//DBG_LB(("逆波兰式结果:%s\n", stOutValue));			
	}

	ret = SYS_STATUS_SUCCESS;

	return ret;
}


/*******************************************************************************/
/* Function Name  : DecodeByFormat                                             */
/* Description    : 解码码收到串口的数据                                       */
/* Input          : BYTE	             		通道号                               */
/*                  BYTE	             		通信编号                             */
/*                  char* sendSrc        	通信协议格式                         */
/*                  char* crc_control    	检验格式                             */
/* Output         : BYTE* sendBuf        	欲发送至串口的数据                   */
/* Return         : int DecodeResult     	解析结果		                         */
/*																				0, 解析失败													 */
/*																				1, 成功, 值域正常										 */
/*																				2, 成功, 值域异常, 不提交						 */
/*																				3, 成功, 值域异常, 提交							 */
/*******************************************************************************/
int LabelIF_DecodeByFormat(PDEVICE_INFO_POLL pDevInfo, unsigned char* pInBuf, int inLen, PQUERY_INFO_CPM_EX stOutBuf)
{
	int ret = SYS_STATUS_DATA_DEOCDE_FAIL;
	char* ptrRD = NULL;

	FORMAT_TIME timeFormat;
	FORMAT_HEAD headFormat;
	FORMAT_PARA rdFormat;
	memset( (unsigned char*)&timeFormat, 0, sizeof(FORMAT_TIME) );
	memset( (unsigned char*)&headFormat, 0, sizeof(FORMAT_HEAD) );
	memset( (unsigned char*)&rdFormat, 0, sizeof(FORMAT_PARA) );
	
	if (0 >= inLen || 0 >= strlen(pDevInfo->RD_Format))//解析格式为空，退出
	{
		DBG(("DecodeByFormat inLen[%d] RdFormatLen[%zu]\n", inLen, strlen(pDevInfo->RD_Format)));
		return SYS_STATUS_DATA_DEOCDE_FAIL;
	}
	
	ptrRD = pDevInfo->RD_Format;
	//
	for (int i=0; i< 2; i++)
	{
		char szRdNode[128] = {0};          //一对大括号之间的内容

		if(0 >= sscanf(ptrRD, "%*[{]%[^}]", szRdNode))
		{
			DBG(("DecodeByFormat 括号格式错误[%s]\n", ptrRD));
			break;
		}

		ptrRD = strstr(ptrRD, szRdNode) + strlen(szRdNode) + 1;
		
		if( 0 == i )               //第一个大括号配置，判断是否先获取时间配置
		{
			GetValueTime(",", szRdNode, &timeFormat);
			if (1 == timeFormat.TI)//是否需要截取时间数据
			{
				memset(stOutBuf->Time, 0, sizeof(stOutBuf->Time));
				if( timeFormat.M == STREAM_CHARS )    //字符串码流
				{
					sprintf(stOutBuf->Time, "%c%c%c%c-%c%c-%c%c %c%c:%c%c:%c%c", pInBuf[timeFormat.Y], pInBuf[timeFormat.Y+1], pInBuf[timeFormat.Y+2], pInBuf[timeFormat.Y+3],
					pInBuf[timeFormat.MO], pInBuf[timeFormat.MO+1], pInBuf[timeFormat.D], pInBuf[timeFormat.D+1], pInBuf[timeFormat.H], pInBuf[timeFormat.H+1],
					pInBuf[timeFormat.MI], pInBuf[timeFormat.MI+1], pInBuf[timeFormat.S], pInBuf[timeFormat.S+1]);
				}
				else if( timeFormat.M == STREAM_BYTES )     //字节流码流
				{
					sprintf(stOutBuf->Time, "%4d-%2d-%2d %2d:%2d:%2d", pInBuf[timeFormat.Y], pInBuf[timeFormat.MO], pInBuf[timeFormat.D], pInBuf[timeFormat.H], pInBuf[timeFormat.MI], pInBuf[timeFormat.S]);
				}
				else if( timeFormat.M == STREAM_BCD )       //BCD码码流
				{
					sprintf(stOutBuf->Time, "%d%d%d%d-%d%d-%d%d %d%d:%d%d:%d%d", pInBuf[timeFormat.Y]>>4, (BYTE)(pInBuf[timeFormat.Y]<<4)>>4, pInBuf[timeFormat.Y+1]>>4, (BYTE)(pInBuf[timeFormat.Y+1]<<4)>>4,
					pInBuf[timeFormat.MO]>>4, (BYTE)(pInBuf[timeFormat.MO]<<4)>>4, pInBuf[timeFormat.D]>>4, (BYTE)(pInBuf[timeFormat.D]<<4)>>4,
					pInBuf[timeFormat.H]>>4, (BYTE)(pInBuf[timeFormat.H]<<4)>>4, pInBuf[timeFormat.MI]>>4, (BYTE)(pInBuf[timeFormat.MI]<<4)>>4, pInBuf[timeFormat.S]>>4, (BYTE)(pInBuf[timeFormat.S]<<4)>>4);
				}
				continue;
			}
		}

		/**************************************数据本身处理**********************************/
		memset( (unsigned char*)&headFormat, 0, sizeof(FORMAT_HEAD) );
		GetHeadValue(",", szRdNode, &headFormat);           //取头部设置
		DBG_LB(("GetHeadValue: id[%s] rd[%s] type[%d]\n", pDevInfo->Id, szRdNode, headFormat.T));
		if( headFormat.ST == -1 )      //码流不正常
		{
			DBG_LB(("DeviceAttrId[%s] DecodeByFormat GetHeadValue failed\n", pDevInfo->Id));
			break;
		}
        
		char szRdMidNode[64] = {0};			//用于存放大括号中中括号内容
		char szCaculateBuf[64] = {0};		//存放最终计算用的表达式
		float midResult[10] = {0};			//如果为计算式，最多支持10个临时值
		int midResultCnt = 0;

		char* midNum = NULL;
		char* savePtr = NULL;
		stOutBuf->Cmd = 0x00;//接收数据

		memcpy(stOutBuf->DevId, pDevInfo->Id,  DEVICE_ID_LEN + DEVICE_SN_LEN);
		memcpy(stOutBuf->Type, headFormat.DT, sizeof(headFormat.DT));

		if (CHARS == headFormat.T || BYTES ==  headFormat.T)
		{
			stOutBuf->VType =  headFormat.T;
		}
		else
		{
			stOutBuf->VType = FLOAT;
		}

		char * ptrRdNode = szRdNode;
		while(NULL != ptrRdNode &&  0 < sscanf(ptrRdNode, "%*[^[][%[^]]", szRdMidNode) )
		{
			ptrRdNode = strstr(ptrRdNode, szRdMidNode) + strlen(szRdMidNode);
			midNum = NULL;
			savePtr = NULL;
			midResultCnt = 0;

			midNum = strtok_r( szRdMidNode, ":", &savePtr);
			if (NULL == midNum)
			{
				DBG_LB(("DeviceAttrId[%s] DecodeByFormat get midNum failed\n", pDevInfo->Id));
				return false;
			}
			midResultCnt = atoi(midNum);
			
			if( midResultCnt == 0 )//判断是否为计算用表达式
			{
				strcpy(szCaculateBuf, savePtr);
				DBG_LB(("DecodeByFormat continued \n"));
				continue;
			}
            
			if( midResultCnt >= 10 )//如果临时值编号超过10个，退出
			{
				DBG_LB(("DeviceAttrId[%s] DecodeByFormat midResultNum[%d] failed\n", pDevInfo->Id, midResultCnt));
				return false;
			}

			GetValue(",", savePtr, &rdFormat);           //获取数值设置
//V2.0.0.6 lhy
			if(1 == rdFormat.LenType)//如果长度是倒序
			{
				rdFormat.DataLen = (inLen - rdFormat.StartBytes - rdFormat.DataLen);
			}
			//printf("DeviceAttrId[%s] DecodeByFormat StartBytes[%d] DataLen[%d] inLen[%d] \n",pDevInfo->Id,  rdFormat.StartBytes, rdFormat.DataLen, inLen);
            //DBG_LB(("inLen:[%d]\n", inLen));
			if( rdFormat.StartBytes + rdFormat.DataLen > inLen || rdFormat.StartBytes == -1 || rdFormat.DataLen == -1 || 0 ==  rdFormat.DataLen)
			{
				DBG_LB(("DeviceAttrId[%s] DecodeByFormat StartBytes[%d] DataLen[%d] inLen[%d] failed\n",pDevInfo->Id,  rdFormat.StartBytes, rdFormat.DataLen, inLen));
				return SYS_STATUS_DATA_DEOCDE_FAIL;
			}
			if( (rdFormat.RT == 2) && ( rdFormat.RL == -1 || rdFormat.R == -1 || rdFormat.R + rdFormat.RL > inLen ))
			{
				DBG_LB(("DeviceAttrId[%s] DecodeByFormat RT[%d] RL[%d] RST[%d] inLen[%d] failed\n", pDevInfo->Id, rdFormat.RT, rdFormat.RL, rdFormat.RST, inLen));
				return SYS_STATUS_DATA_DEOCDE_FAIL;
			}

			BYTE tempdata[MAX_PACKAGE_LEN_TERMINAL] = {0};
			int tValue = 0;
			int tResult = 0;
			int tCnt = 0;
			float ratio = 0;
			BYTE tempSwap = 0;
			unsigned char tempBCD2[1024] = {0};

			if( rdFormat.DataLen > 0 )
			{
				memcpy(tempdata, pInBuf + rdFormat.StartBytes, rdFormat.DataLen);

				DBG_LB(("DecodeByFormat Temp Value rdFormat.DataType[%d] rdFormat.ChangeBytes[%d] headFormat.ST[%d] stOutBuf->VType[%d]\n", rdFormat.DataType,rdFormat.ChangeBytes, headFormat.ST, stOutBuf->VType));
				//AddMsg(tempdata, rdFormat.DataLen);

				if( rdFormat.ChangeBytes == 1 && STREAM_BCD_2 != headFormat.ST)    //高低字节转换
				{
					for( int j=0; j<rdFormat.DataLen/2; j++ )
					{
						tempSwap = tempdata[j];
						tempdata[j] = tempdata[rdFormat.DataLen - 1 - j];
						tempdata[rdFormat.DataLen - 1 - j] = tempSwap;
					}
				}
				if( headFormat.ST == STREAM_BYTES_33 )     //电能表字节流
				{
					for( int j=0; j<rdFormat.DataLen; j++ )
					{
						tempdata[j] -= 0x33;
					}
				}
				//printf("tempData:[%02x %02x %02x %02x]\n", tempdata[0], tempdata[1], tempdata[2], tempdata[3]);

				/************************************获取基础数据***********************************/
				switch(rdFormat.DataType)
				{
				case CHARS://字符串
				case BYTES://字节码
					if(STREAM_CHARS == headFormat.ST || STREAM_BCD_2 == headFormat.ST)
						memcpy(stOutBuf->Value, tempdata, rdFormat.DataLen);
					else
						Hex2String( stOutBuf->Value, (char *)tempdata, rdFormat.DataLen*2);
					break;
				case SHORT://短整形
					if(STREAM_CHARS == headFormat.ST)    //字符流
					{
						DBG_LB(("trim Value[%s]\n", trim((char*)tempdata, strlen((char*)tempdata))));		
						if (0 >= strlen(trim((char*)tempdata, strlen((char*)tempdata))))
							return SYS_STATUS_DATA_DEOCDE_FAIL;
						midResult[midResultCnt] = atoi(trim((char*)tempdata, strlen((char*)tempdata)))*1.0;
					}
					else if(STREAM_BCD_2 == headFormat.ST)
					{
						memset(tempBCD2, 0, sizeof(tempBCD2));
						if(1 > String2Hex(tempBCD2, tempdata, rdFormat.DataLen))
						{
							DBG(("DecodeByFormat 111 String2Hex Failed\n"));
							return SYS_STATUS_DATA_DEOCDE_FAIL;
						}
						int tLen = rdFormat.DataLen/2;
						if( rdFormat.ChangeBytes == 1)    //高低字节转换
						{
							for( int j=0; j<tLen/2; j++ )
							{
								tempSwap = tempBCD2[j];
								tempBCD2[j] = tempBCD2[tLen - 1 - j];
								tempBCD2[tLen - 1 - j] = tempSwap;
							}
						}

						//DBG_LB(("STREAM_BCD_2 111 String2Hex TLen[%d] rdFormat.ChangeBytes[%d]\n", tLen, rdFormat.ChangeBytes));
						AddMsg(tempBCD2, 2);

						midResult[midResultCnt] = *((short*)tempBCD2)*1.0;
					}
					else if(STREAM_BYTES == headFormat.ST)   //字节流
						midResult[midResultCnt] = ByteToShort(tempdata)*1.0;
					else if(STREAM_BCD == headFormat.ST || STREAM_BYTES_33 == headFormat.ST)     //BCD流
					{
						for( tCnt=0; tCnt<rdFormat.DataLen; tCnt++ )
						{
							tResult *= 100;
							tValue = (tempdata[tCnt] >> 4) * 10 + ((BYTE)(tempdata[tCnt] << 4) >> 4);
							tResult += tValue;
						}
						midResult[midResultCnt] = tResult*1.0;
					}
					else if(STREAM_CHARS_EMERSON == headFormat.ST)					 //艾默生 (字符转字节）	lsd 2015.12.29
					{
						DBG_LB(("trim Value[%s]\n", trim((char*)tempdata, strlen((char*)tempdata))));		
						if (0 >= strlen(trim((char*)tempdata, strlen((char*)tempdata))))					
						{
							DBG_LB(("STREAM_CHARS_EMERSON == headFormat.ST failed \n"));
							return SYS_STATUS_DATA_DEOCDE_FAIL;
						}
						//转成16进制 
						BYTE tmpShort[2] = {0};												//lsd 2015.12.29
						//该函数不包含高低字节转换（长度小于4）
						String2Hex((unsigned char*)tmpShort, (unsigned char*)tempdata, 4);	//lsd 2015.12.29
						//高低字节转换
						tempSwap = tmpShort[0];
						tmpShort[0] = tmpShort[1];
						tmpShort[1] = tempSwap;
						midResult[midResultCnt] = ByteToShort(tmpShort)*1.0;				//lsd 2015.12.29
					}
					else
						midResult[midResultCnt] = ByteToShort(tempdata)*1.0;
					break;
				case INTEGER://整形
					if(STREAM_CHARS == headFormat.ST)
					{
						if (0 >= strlen(trim((char*)tempdata, strlen((char*)tempdata))))
							return SYS_STATUS_DATA_DEOCDE_FAIL;
						midResult[midResultCnt] = atoi(trim((char*)tempdata, strlen((char*)tempdata)))*1.0;
					}
					else if(STREAM_BCD_2 == headFormat.ST)
					{
						memset(tempBCD2, 0, sizeof(tempBCD2));
						if(1 > String2Hex(tempBCD2, tempdata, rdFormat.DataLen))
							return SYS_STATUS_DATA_DEOCDE_FAIL;
						midResult[midResultCnt] = *((int*)tempBCD2)*1.0;
					}
					else if(STREAM_BYTES == headFormat.ST)
						midResult[midResultCnt] = *((int*)tempdata)*1.0;
					else if(STREAM_BCD == headFormat.ST || STREAM_BYTES_33 == headFormat.ST)       //BCD码  33流电能表专用，BCD计算
					{
						for( tCnt=0; tCnt<rdFormat.DataLen; tCnt++ )
						{
							tResult *= 100;
							tValue = (tempdata[tCnt] >> 4) * 10 + ((BYTE)(tempdata[tCnt] << 4) >> 4);
							tResult += tValue;
						}
						midResult[midResultCnt] = tResult*1.0;
					}
					else
						midResult[midResultCnt] = *((int*)tempdata)*1.0;
					break;
				case CHAR:    //字符
					if(STREAM_CHARS == headFormat.ST)
					{
						if (0 >= strlen(trim((char*)tempdata, strlen((char*)tempdata))))
							return SYS_STATUS_DATA_DEOCDE_FAIL;
						midResult[midResultCnt] = atoi(trim((char*)tempdata, strlen((char*)tempdata)))*1.0;
					}
					else if(STREAM_BCD_2 == headFormat.ST)
					{
						memset(tempBCD2, 0, sizeof(tempBCD2));
						if(1 > String2Hex(tempBCD2, tempdata, rdFormat.DataLen))
							return SYS_STATUS_DATA_DEOCDE_FAIL;
						midResult[midResultCnt] = *((char*)tempBCD2)*1.0;
					}
					else if(STREAM_BYTES == headFormat.ST)
					{
						midResult[midResultCnt] = *((char*)tempdata)*1.0;
						
					}
					else if(STREAM_BCD == headFormat.ST)       //BCD码
					{
						for( tCnt=0; tCnt<rdFormat.DataLen; tCnt++ )
						{
							tResult *= 100;
							tValue = (tempdata[tCnt] >> 4) * 10 + ((BYTE)(tempdata[tCnt] << 4) >> 4);
							tResult += tValue;
						}
						midResult[midResultCnt] = *((char*)tResult)*1.0;
					}
					else
						midResult[midResultCnt] = *((char*)tempdata)*1.0;
					if(midResult[midResultCnt] > 128)
						midResult[midResultCnt] -= 256;
					break;
				case UCHAR:    //字符
					if(STREAM_CHARS == headFormat.ST)
					{
						if (0 >= strlen(trim((char*)tempdata, strlen((char*)tempdata))))
							return SYS_STATUS_DATA_DEOCDE_FAIL;
						midResult[midResultCnt] = atoi(trim((char*)tempdata, strlen((char*)tempdata)))*1.0;
					}
					else if(STREAM_BCD_2 == headFormat.ST)
					{
						memset(tempBCD2, 0, sizeof(tempBCD2));
						if(1 > String2Hex(tempBCD2, tempdata, rdFormat.DataLen))
							return SYS_STATUS_DATA_DEOCDE_FAIL;
						midResult[midResultCnt] = *((unsigned char*)tempBCD2)*1.0;
					}
					else if(STREAM_BYTES == headFormat.ST)
						midResult[midResultCnt] = *((unsigned char*)tempdata)*1.0;
					else if(STREAM_BCD == headFormat.ST)       //BCD码
					{
						for( tCnt=0; tCnt<rdFormat.DataLen; tCnt++ )
						{
							tResult *= 100;
							tValue = (tempdata[tCnt] >> 4) * 10 + ((BYTE)(tempdata[tCnt] << 4) >> 4);
							tResult += tValue;
						}
						midResult[midResultCnt] = *((unsigned char*)tResult)*1.0;
					}
					else
						midResult[midResultCnt] = *((unsigned char*)tempdata)*1.0;
					break;
				case FLOAT:
				case DOUBLE:    //浮点型
					if(STREAM_CHARS == headFormat.ST)
					{
						DBG_LB(("trim Value[%s]\n", trim((char*)tempdata, strlen((char*)tempdata))));
						if (0 >= strlen(trim((char*)tempdata, strlen((char*)tempdata))))
						{
							DBG_LB(("STREAM_CHARS == headFormat.ST failed \n"));
							return SYS_STATUS_DATA_DEOCDE_FAIL;
						}
						midResult[midResultCnt] = atof(trim((char*)tempdata, strlen((char*)tempdata)))*1.0;
					}
					else if(STREAM_BCD_2 == headFormat.ST)
					{
						memset(tempBCD2, 0, sizeof(tempBCD2));
						if(1 > String2Hex(tempBCD2, tempdata, rdFormat.DataLen))
						{
							DBG_LB(("1 > String2Hex(tempBCD2, tempdata, rdFormat.DataLen)\n"));
							return SYS_STATUS_DATA_DEOCDE_FAIL;
						}
						midResult[midResultCnt] = *((float*)tempBCD2);
					}
					else if(STREAM_BYTES == headFormat.ST || STREAM_BYTES_33 == headFormat.ST)	
					{
						BYTE tmpFloat[4] = {0};												//lsd 2015.7.3
						memcpy((unsigned char*)tmpFloat, tempdata, sizeof(float));			//lsd 2015.7.3

						printf("tmpFloat:[%02x %02x %02x %02x]\n", tmpFloat[0], tmpFloat[1], tmpFloat[2], tmpFloat[3]);

					//	midResult[midResultCnt] = *((float*)tmpFloat);						//lsd 2015.7.3
						



						midResult[midResultCnt] = *((float*)tempdata);
					}
					else if(STREAM_BCD == headFormat.ST)       //BCD流
					{
						for( tCnt=0; tCnt<rdFormat.DataLen; tCnt++ )
						{
							tResult *= 100;
							tValue = (tempdata[tCnt] >> 4) * 10 + ((BYTE)(tempdata[tCnt] << 4) >> 4);
							tResult += tValue;
						}
						midResult[midResultCnt] = tResult*1.0;
					}
					else if(STREAM_CHARS_EMERSON == headFormat.ST)					 //艾默生UPS (转hex）	lsd 2015.7.6
					{
						DBG_LB(("trim Value[%s]\n", trim((char*)tempdata, strlen((char*)tempdata))));		
						if (0 >= strlen(trim((char*)tempdata, strlen((char*)tempdata))))					
						{
							DBG_LB(("STREAM_CHARS_EMERSON == headFormat.ST failed \n"));
							return SYS_STATUS_DATA_DEOCDE_FAIL;
						}
						//转成16进制
						BYTE tmpFloat[4] = {0};												//lsd 2015.7.6
						//该函数包含高低字节转换
						String2Hex((unsigned char*)tmpFloat, (unsigned char*)tempdata, 8);	//lsd 2015.7.6
						//转成Float  IEEE-745标准 （32位float浮点数）
						midResult[midResultCnt] = *((float*)tmpFloat);						//lsd 2015.7.6
					}
					else
					{
						midResult[midResultCnt] = *((float*)tempdata);
					}
					break;
				case BIT:
					midResult[midResultCnt] = (tempdata[0] >> rdFormat.BitPos)&0x01;
					break;
				default:
					DBG_LB(("DeviceAttrId[%s] DecodeByFormat DataType[%d] error\n", pDevInfo->Id, rdFormat.DataType));
					return SYS_STATUS_DATA_DEOCDE_FAIL;
				}
			}
			
			DBG_LB(("--------111--------midResult[%f]\n",midResult[midResultCnt]));

			/***********************************获取系数值********************************************/
			BYTE table_type = 0;        //参数表种类
			int len = 0;
			char szTableList[1024] = {0};
			FORMAT_MAP_TABLE mapTable[200] = {{0}};
      if( rdFormat.RT == 0 )               //系数是一个常量
      {
          ratio = rdFormat.R;
      }
      else if( rdFormat.RT == 1 )          //系数需要常量查表
      {
        GetCalLable(pDevInfo->RD_Comparison_table, headFormat.DT, midResultCnt, len, szTableList, &table_type);
        if( table_type != 1 && table_type != 2 )
        {
					DBG_LB(("------rdFormat.RT == 1 exit\n"));
					return SYS_STATUS_DATA_DEOCDE_FAIL;
				}
        int cnt = GenerateTable(";", "~", szTableList, mapTable, 200);
        GetValueByTable(1, rdFormat.R, table_type, NULL, &rdFormat, headFormat, mapTable, cnt, &ratio);
			}
      else if( rdFormat.RT == 2 )          //系数需要截取值查表
      {
        GetCalLable(pDevInfo->RD_Comparison_table, headFormat.DT, midResultCnt, len, szTableList, &table_type);
        if( table_type != 1 && table_type != 2 )
				{
					DBG_LB(("------rdFormat.RT == 2 exit\n"));
					return SYS_STATUS_DATA_DEOCDE_FAIL;
				}
        int cnt = GenerateTable(";", "~", szTableList, mapTable, 200);
        GetValueByTable(2, 0, table_type, pInBuf, &rdFormat, headFormat, mapTable, cnt, &ratio);
			}

			DBG_LB(("R result:[%f] rdFormat.DC[%s]\n", ratio,  rdFormat.DC));

      //确定实际值计算方法   D:直接截取实际值    R:直接用系数作为实际值    DR:截取值与系数相乘得实际值
      if( rdFormat.DC[0] == 'D' &&  rdFormat.DC[1] == 0 )
      {
      }
      else if( rdFormat.DC[0] == 'R' )
      {
          midResult[midResultCnt] = ratio;
      }
      else if( rdFormat.DC[0] == 'D' && rdFormat.DC[1] == 'R' )
      {
          midResult[midResultCnt] *= ratio;
      }
      else
      {				
					DBG_LB(("------rdFormat.DC[%s]  exit\n", rdFormat.DC));
          return SYS_STATUS_DATA_DEOCDE_FAIL;
      }
		}
        
		/**********************************正则表达式计算***************************************/
		if( stOutBuf->VType == FLOAT )
		{
      char tempCalFunc[128] = {0};
      float result = 0;
			//DBG_LB(("逆波兰式:%s\n", szCaculateBuf));
			transPolish(szCaculateBuf, tempCalFunc);
			//DBG_LB(("逆波兰式转换后:%s headFormat.T[%d]\n", tempCalFunc, headFormat.T));
			result = compvalue(tempCalFunc, midResult);
			switch(headFormat.T)
			{
			case SHORT://短整形
				sprintf(stOutBuf->Value, "%d", (short)result);
				break;
			case INTEGER://整形
				sprintf(stOutBuf->Value, "%d", (int)result);
				break;
			case CHAR:
				sprintf(stOutBuf->Value, "%d", (char)result);
				break;	
			case UCHAR:
				sprintf(stOutBuf->Value, "%d", (unsigned char)result);
				break;
			default:
				sprintf(stOutBuf->Value, "%.2f", result);
				break;
			}
			//DBG_LB(("逆波兰式结果:%s\n", stOutBuf->Value));		
		}
		stOutBuf->Value[sizeof(stOutBuf->Value)-1] = '\0';
		//值域范围判断
		

		int iRangeCheck = LabelIF_RangeCheck(pDevInfo, stOutBuf->VType, stOutBuf->Value, pDevInfo->stLastValueInfo.szValueUseable);
		ret = iRangeCheck;		
		break;
	}
	printf("LabelIF_Decode Result[%d]\n", ret);
	return ret;
}
/******************************************************************************/
// Function Name  : LabelIF_RangeCheck                                               
// Description    : 值域 & 离线次数 & 标准范围                                         
// Input          : 
//
//			**************值    域*********************;
//										0,0,100,0										- 范围型,最   小   值,最   大   值,是否记录(0,按离线处理;1,记录)
//									  1,0,50000000,0							-	递增型,递增范围 min,递增范围 max,是否记录(0,按离线处理;1,记录)
//									  2,999999999999,50000000,0		- 递减型,递减范围 min,递减范围 max,是否记录(0,按离线处理;1,记录)
//			************离线次数***********************;
// Output         : 
// Return         : 0 正常数据
//									1 超出值域范围，不交予AGENT处理
//									2 超出值域范围，交予AGENT处理
/******************************************************************************/
int LabelIF_RangeCheck(PDEVICE_INFO_POLL pDevInfo, int vType, char* recValue, char* lastValue)
{
	bool retNormal = false;
	//解析1
	if( strlen(pDevInfo->szValueLimit) <= 1 )     //未设定标准范围时，认为数据正常
	{
		return SYS_STATUS_DATA_DECODE_NORMAL;
	}
	//定义
	//
	int recValue_interg = 0;           								//轮询取得的值
	double recValue_doub = 0;	
	int dbValue_interg = atoi(lastValue);           	//上一个值
	double dbValue_doub = atof(lastValue);
	//解析recValue
	if( vType == INTEGER )
	{
		recValue_interg = atoi(recValue);
		//DBG(("RecValue:[%d]", recValue_interg));
	}
	else if( vType == FLOAT )
	{
		recValue_doub = atof(recValue);
		//DBG(("RecValue:[%f]", recValue_doub));
	}

	if(0 == pDevInfo->Range_Type)//范围型
	{
		if( vType == INTEGER )
		{
			if(recValue_interg > pDevInfo->Range_1 && recValue_interg < pDevInfo->Range_2)
				retNormal = true;
		}
		else if( vType == FLOAT )
		{
			if(recValue_doub > pDevInfo->Range_1 && recValue_doub < pDevInfo->Range_2)
				retNormal = true;
		}
	}
	else if(1 == pDevInfo->Range_Type)//递增型
	{
		if( vType == INTEGER )
		{
			if(recValue_interg - dbValue_interg > pDevInfo->Range_1 && recValue_interg - dbValue_interg < pDevInfo->Range_2)
				retNormal = true;
		}
		else if( vType == FLOAT )
		{
			if(recValue_doub - dbValue_doub > pDevInfo->Range_1 && recValue_doub - dbValue_doub < pDevInfo->Range_2)
				retNormal = true;
		}		
	}
	else if(2 == pDevInfo->Range_Type)//递减型
	{
		if( vType == INTEGER )
		{
			if(dbValue_interg - recValue_interg > pDevInfo->Range_1 && dbValue_interg - recValue_interg < pDevInfo->Range_2)
				retNormal = true;
		}
		else if( vType == FLOAT )
		{
			if(dbValue_doub - recValue_doub > pDevInfo->Range_1 && dbValue_doub - recValue_doub < pDevInfo->Range_2)
				retNormal = true;
		}
	}
	
	if(retNormal)	//正常
		return SYS_STATUS_DATA_DECODE_NORMAL;
	else					//异常
		return SYS_STATUS_DATA_DECODE_UNNOMAL_THROW + pDevInfo->Range_Agent;
}


/******************************************************************************/
// Function Name  : LabelIF_StandarCheck                                               
// Description    : 值域 & 离线次数 & 标准范围                                         
// Input          : 
//
//			************标准定义***********************;
//										>1,<100											- 区间1	
//										>40													- 区间2
//										<60													- 区间3
//										=0													- 等于
//									 !=0													- 不等于
// Output         : 
// Return         : time_t 下次动作时间                                                     
/******************************************************************************/
bool LabelIF_StandarCheck(int vType, char* value, char* standard)
{
	bool ret = false;
	if( strlen(standard) <= 0 )     //未设定标准范围时，认为数据正常
	{
		return true;
	}

	const char* split = ",";
	char* split_result = NULL;
	int recValue_interg = 0;           //轮询取得的值
	double recValue_doub = 0;
	int conValue_interg = 0;           //基准值
	double conValue_doub = 0;

	if( vType == INTEGER )
	{
		recValue_interg = atoi(value);
		//DBG(("RecValue:[%d]", recValue_interg));
	}
	else if( vType == FLOAT )
	{
		recValue_doub = atof(value);
		//DBG(("RecValue:[%f]", recValue_doub));
	}

	char* savePtr = NULL;
	split_result = strtok_r(standard, ",", &savePtr);
	while(split_result != NULL)
	{
		//DBG(("Condition:[%s]\n", split_result));
		if( *split_result == '*' )        //首字节为 *,表示任何值均使条件成立
		{
			return true;
		}
		else if( *split_result == '<' )
		{
			if( vType == INTEGER )
			{
				conValue_interg = atoi(split_result+1);
				//DBG(("convalue:[%d]\n", conValue_interg));
				if( recValue_interg < conValue_interg ){    ret = true;  }
				else{    return false;  }    //任何条件不满足，均为条件不成立
			}
			else if( vType == FLOAT )
			{
				conValue_doub = atof(split_result+1);
				//DBG(("convalue <:[%f]\n", conValue_doub));
				if( recValue_doub < conValue_doub ){    ret = true;  }
				else{    return false;  }
			}
		}
		else if( *split_result == '>' )
		{
			if( vType == INTEGER )
			{
				conValue_interg = atoi(split_result+1);
				DBG_LB(("convalue:[%d]\n", conValue_interg));
				if( recValue_interg > conValue_interg ){    ret = true;  }
				else{    return false;  }
			}
			else if( vType == FLOAT )
			{
				conValue_doub = atof(split_result+1);
				DBG_LB(("convalue >:[%f]\n", conValue_doub));
				if( recValue_doub > conValue_doub ){    ret = true;  }
				else{    return false;  }
			}
		}
		else if( *split_result == '=' )
		{
			if( vType == INTEGER )
			{
				conValue_interg = atoi(split_result+1);
				//DBG_LB(("convalue:[%d]\n", conValue_interg));
				if( recValue_interg == conValue_interg ){    ret = true;  }
				else{    return false;  }
			}
			else if( vType == FLOAT )
			{
				conValue_doub = atof(split_result+1);
				//DBG_LB(("convalue =:[%f]\n", conValue_doub));
				if( recValue_doub == conValue_doub ){    ret = true;  }
				else{    return false;  }
			}
		}

		split_result = strtok_r(NULL, split, &savePtr);
	}
	return ret;
}

void LabelIF_GetDeviceValueLimit(char* config_str, PDEVICE_INFO_POLL pDeviceInfoPoll)
{	
	if (strlen(config_str) < 1)
	{
		return;
	}

	char offLineTotal[3] = {0};
	char cRange_1[32] = {0};
	char cRange_2[32] = {0};

	if (config_str == strstr(config_str, ";"))	//";"前没有任何字符
	{
		sscanf(config_str, ";%[0-9];", offLineTotal);
		pDeviceInfoPoll->offLineTotle = atoi(offLineTotal);
		pDeviceInfoPoll->offLineCnt = pDeviceInfoPoll->offLineTotle;
	}
	else
	{
		sscanf(config_str, "%[^;];%[0-9];", pDeviceInfoPoll->szValueLimit, offLineTotal);
		pDeviceInfoPoll->offLineTotle = atoi(offLineTotal);
		pDeviceInfoPoll->offLineCnt = pDeviceInfoPoll->offLineTotle;
		sscanf(pDeviceInfoPoll->szValueLimit, "%d,%[^,],%[^,],%d;", &pDeviceInfoPoll->Range_Type, cRange_1, cRange_2, &pDeviceInfoPoll->Range_Agent);
		pDeviceInfoPoll->Range_1 = atof(cRange_1);
		pDeviceInfoPoll->Range_2 = atof(cRange_2);
	}	
}
