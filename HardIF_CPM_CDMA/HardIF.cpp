/******************************************************************************/
/*    File    : HardIF.cpp   VERSION:CPM_V1                                   */
/*    Author  :                                                               */
/*    Creat   :                                                               */
/*    Function:                                                               */
/******************************************************************************/

/******************************************************************************/
/*                       头文件                                               */
/******************************************************************************/
#include <sys/time.h>
#include <sys/ioctl.h>
#include <string>
#include <stdlib.h>
#include <error.h>
#include "HardIF.h"

#include "IniFile.h"
#include "GSMBaseSiemens.h"
#include "LightCtrl.h"
#include "Sys485.h"
#include "ADCBase.h"
#include "ADProc.h"
#include "EmApi.h"


#ifdef  DEBUG
//#define DEBUG_IF
#endif

#ifdef DEBUG_IF
#define DBG_IF(a)		printf a;
#else
#define DBG_IF(a)	
#endif

/******************************************************************************/
/*                       局部宏定义                                           */
/******************************************************************************/
CADCBase	g_CADCBaseA(1);/*/dev/spidev1.1口*/
CADCBase	g_CADCBaseB(0);/*/dev/spidev1.2口*/

CADProc		g_CADProc(&g_CADCBaseA, &g_CADCBaseB);

CLightCtrl	g_LightCtrl;
int			g_hdlGPIO;

CGSMBaseSiemens g_GSMBaseSiemens;
/******************************************************************************/
/*                       局部函数                                             */
/******************************************************************************/
//GPIO模块初始化
bool GPIO_Port_Init(char* pGPIODevName) 
{ 
	DBG(("\nGpio 初始化 [%s]", pGPIODevName));
	
	DBG((". "));
	//GPIO
	if (0 != EMClose(g_hdlGPIO))
	{
		DBG(("失败\n"));
		return false;
	}
	DBG((". "));
	if (0 != EMOpenWithName(&g_hdlGPIO, pGPIODevName))
	{
		DBG(("失败\n"));
		return false;
	}
	DBG((". 成功\n"));
	return true;
} 



/******************************************************************************/
/*                       公共函数                                             */
/******************************************************************************/
//初始化（自起线程）
bool HardIF_Initialize(ADCValueNotice_CallBack ADCNotic_CallBack, void* userData, STR_SMS_CFG strSmsCfg)
{
	char szEMPath[56] = {0};
	sprintf(szEMPath, "/dev/EM");
	
	char szSPIPath1[56] = {0};
	char szSPIPath2[56] = {0};
	sprintf(szSPIPath1, "/dev/spidev1.1");
	sprintf(szSPIPath2, "/dev/spidev1.2");

	//GSM
	char szGSMDev[56] = {0};
	char szSMSCenter[56] = {0};
	int iGsmBaudRate = 9600;
	sprintf(szGSMDev, "/dev/ttyS1");
	sprintf(szSMSCenter, "8613800571500");
	get_ini_string("Config.ini","SYSTEM","GSMCenter" ,"=", szSMSCenter, sizeof(szSMSCenter));
	
	//2.1 GPIO设备
	if (false == GPIO_Port_Init(szEMPath)) return false;
	
	//2.2 spi设备
	if (false == g_CADCBaseA.Initialize(szSPIPath1))return false;
	if (false == g_CADCBaseB.Initialize(szSPIPath2))return false;

	//2.3 开关量模拟量线程初始化
	if (false == g_CADProc.Initialize(ADCNotic_CallBack, userData))return false;

	//2.4 灯控
	if(false == g_LightCtrl.Initialize(g_hdlGPIO, &g_CADCBaseA, &g_CADCBaseB))return false;

	//2.5 短信模块初始化
	if (!g_GSMBaseSiemens.Initialize(szGSMDev, iGsmBaudRate, szSMSCenter, strSmsCfg.ulHandleGSMSend, strSmsCfg.ulHandleGSMRecv))
	{
		DBG(("GSMBaseSiemens Initialize failed!\n"));
		return false;
	}
	return true;
}
bool HardIF_Terminate(void)
{
	EMClose(g_hdlGPIO);
	return true;
}

//开关量、模拟量
int HardIF_GetADValue(int Id, float& fInValue)
{
	return g_CADProc.GetADValue(Id, fInValue);
}
int HardIF_SetADValue(int Id, int iOutValue)
{
	return g_CADProc.SetADValue(Id, iOutValue);
}

//GSM
int HardIF_DoGSMAction(PGSM_MSG pGsmMsg)
{
	return g_GSMBaseSiemens.SetGsmSend(pGsmMsg);
}

int HardIF_GetGSMMsg(GSM_MSG& outMsg)
{
	return g_GSMBaseSiemens.GetGsmRecv(outMsg);
}

int	HardIF_DeleteSms(int iIndex)
{
	return g_GSMBaseSiemens.DeleteSms(iIndex);
}

char* HardIF_GetGSMIMSI()
{
	return g_GSMBaseSiemens.GetIMSI();
}
//灯控制
void 	HardIF_BackLightSet( int portNum, int status )
{
	CLightCtrl::BackLightSet(portNum, status);
}
void 	HardIF_AllLightOFF()
{
	CLightCtrl::AllLightOFF();
}
void 	HardIF_AllLightON()
{
	CLightCtrl::AllLightON();
}
void 	HardIF_GSMLightOFF()
{
	CLightCtrl::GSMLightOFF();
}
void 	HardIF_GSMLightON()
{
	CLightCtrl::GSMLightON();
}
void	HardIF_DBGLightOFF()
{
	CLightCtrl::DBGLightOFF();
}
void	HardIF_DBGLightON()
{
	CLightCtrl::DBGLightON();
}
void	HardIF_PLALightOFF()
{
	CLightCtrl::PLALightOFF();
}
void	HardIF_PLALightON()
{
	CLightCtrl::PLALightON();
}
//其他GPIO操作
int HardIF_GetGD0()
{
	return CLightCtrl::GetGD0();
}

/******************************************************************************/
/*                    CHardIF_485 函数                                        */
/******************************************************************************/

CHardIF_485::CHardIF_485(int upperId, int iBaudRate, int iDataBytes, char cCheckMode, int iStopBit, int iReadTimeOut, int typeProc)
{
	m_upperId = upperId;
	m_iBaudRate = iBaudRate;
	m_iDataBytes = iDataBytes;
	m_cCheckMode = cCheckMode;
	m_iStopBit = iStopBit;
	m_iReadTimeOut = iReadTimeOut;
	m_typeProc = typeProc;
}

CHardIF_485::~CHardIF_485(void)
{		

}

int CHardIF_485::Exec485(unsigned char* respBuf, int& nRespLen, BYTE* reqBuf, int nReqLen)
{
	int ret = SYS_STATUS_FAILED;
	int iSendLen = 0;
	int nHandle = -1;
	switch(m_upperId)
	{				
	case INTERFACE_RS485_1:
	case INTERFACE_RS485_2:
		{
			if(SYS_STATUS_SUCCESS != OpenComm(m_upperId, m_iBaudRate, m_iDataBytes, m_cCheckMode, m_iStopBit, m_iReadTimeOut, nHandle))
			{			
				ret = SYS_STATUS_DEVICE_COMMUNICATION_ERROR;
				DBG(("CDeviceIfSys485::OpenComm Failed!\n"));
				break;
			}
			//向串口发送数据
			if ( nReqLen != (iSendLen = SendCommMsg(nHandle, reqBuf, nReqLen)))
			{
				ret = SYS_STATUS_DEVICE_COMMUNICATION_ERROR;
				DBG(("CDeviceIfSys485::SendCommMsg Failed!\n"));
				break;
			}

			//接收回应数据
			nRespLen = RecvCommMsg(nHandle, respBuf, MAX_PACKAGE_LEN_TERMINAL, m_iReadTimeOut);
			
			if (0 < nRespLen && MAX_PACKAGE_LEN_TERMINAL > nRespLen)
			{
				ret = SYS_STATUS_SUCCESS;
				DBG(("CDeviceIfSys485::RecvCommMsg Success! upperId[%d] reqBuf[", m_upperId));
				int i=0;
				for(i=0; i<nReqLen; i++)
				{
					DBG((" %02x", reqBuf[i]));
				}
				DBG(("]\n"));
			}
			else
			{
				ret = SYS_STATUS_DEVICE_COMMUNICATION_ERROR;
				DBG(("CDeviceIfSys485::RecvCommMsg Failed! iReadTimeOut[%d]\n", m_iReadTimeOut));
			}
			if (-1 != nHandle)
			{
				CloseComm(nHandle);
				nHandle = -1;
			}
		}
			break;
	case INTERFACE_RS485_8:
	case INTERFACE_RS485_4:
	case INTERFACE_RS485_6:
		{
			int baudRateEnum = CADCBase::f_getBaudRateEnum( m_iBaudRate );
			g_CADCBaseA.SetBaudRate(m_upperId, (ADC_BAUDRATE)baudRateEnum);
			if( m_cCheckMode == 'O' )
			{
				g_CADCBaseA.SetSerialOpt(m_upperId, m_iStopBit, 1);
			}
			else if( m_cCheckMode == 'E' )
			{
				g_CADCBaseA.SetSerialOpt(m_upperId, m_iStopBit, 2);
			}
			else
			{
				g_CADCBaseA.SetSerialOpt(m_upperId, m_iStopBit, 0);
			}
			do
			{
				//向串口发送数据
				if (nReqLen > g_CADCBaseA.SerialSend(m_upperId, reqBuf, nReqLen))
				{
					ret = SYS_STATUS_DEVICE_COMMUNICATION_ERROR;
					DBG(("CDeviceIfSpi485::g_CADCBaseA SerialSend Failed!\n"));
					break;
				}			
				
				nRespLen = g_CADCBaseA.SerialRecv(m_upperId, respBuf, MAX_PACKAGE_LEN_TERMINAL, m_iReadTimeOut);

				if (nRespLen > 0)
				{
					ret = SYS_STATUS_SUCCESS;
					DBG(("CDeviceIfSpi485::g_CADCBaseA SerialRecv Success! upperId[%d] respBuf[", m_upperId));
					int i=0;
					for(i=0; i<nRespLen; i++)
					{
						DBG((" %02x", respBuf[i]));
					}
					DBG(("]\n"));
				}
				else
				{
					ret = SYS_STATUS_DEVICE_COMMUNICATION_ERROR;
					DBG(("CDeviceIfSpi485::g_CADCBaseA SerialRecv Failed! upperId[%d] reqBuf[", m_upperId));
					
					int i=0;
					for(i=0; i<nReqLen; i++)
					{
						DBG((" %02x", reqBuf[i]));
					}
					DBG(("]\n"));
				}
			}while(false);
		}
		break;

		//485接口B组
	case INTERFACE_RS485_7:
	case INTERFACE_RS485_3:
	case INTERFACE_RS485_5:
		{
			int baudRateEnum = CADCBase::f_getBaudRateEnum( m_iBaudRate );
			g_CADCBaseB.SetBaudRate(m_upperId, (ADC_BAUDRATE)baudRateEnum);
			if( m_cCheckMode == 'O' )
			{
				g_CADCBaseB.SetSerialOpt(m_upperId, m_iStopBit, 1);
			}
			else if( m_cCheckMode == 'E' )
			{
				g_CADCBaseB.SetSerialOpt(m_upperId, m_iStopBit, 2);
			}
			else
			{
				g_CADCBaseB.SetSerialOpt(m_upperId, m_iStopBit, 0);
			}
			do			
			{
				if (nReqLen > g_CADCBaseB.SerialSend(m_upperId, reqBuf, nReqLen))
				{
					ret = SYS_STATUS_DEVICE_COMMUNICATION_ERROR;
					DBG(("CDeviceIfSpi485::g_CADCBaseB SerialSend Failed!\n"));
					break;
				}
	
				nRespLen = g_CADCBaseB.SerialRecv(m_upperId, respBuf, MAX_PACKAGE_LEN_TERMINAL, m_iReadTimeOut);
				
				if (nRespLen > 0)
				{
					ret = SYS_STATUS_SUCCESS;
					DBG(("CDeviceIfSpi485::g_CADCBaseB SerialRecv Success! upperId[%d] respBuf[", m_upperId));
					int i=0;
					for(i=0; i<nRespLen; i++)
					{
						DBG((" %02x", respBuf[i]));
					}
					DBG(("]\n"));
				}
				else
				{
					ret = SYS_STATUS_DEVICE_COMMUNICATION_ERROR;
					DBG(("CDeviceIfSpi485::g_CADCBaseB SerialRecv Failed! upperId[%d] reqBuf[", m_upperId));
					
					int i=0;
					for(i=0; i<nReqLen; i++)
					{
						DBG((" %02x", reqBuf[i]));
					}
					DBG(("]\n"));
				}    
			}while(false);
		}
		break;
	}
	return ret;
}

