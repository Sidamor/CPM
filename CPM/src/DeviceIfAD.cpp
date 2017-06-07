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
#include <stdio.h>
#include "Shared.h"
#include "Init.h"
#include "SqlCtl.h"
#include "DeviceIfAD.h"
#include "HardIF.h"
#include "LabelIF.h"


/******************************************************************************/
/*                       局部宏定义                                           */
/******************************************************************************/

/******************************************************************************/
/*                       局部函数                                             */
/******************************************************************************/

CDeviceIfAD::CDeviceIfAD()
{
	memset(m_szCurDeviceAttrId, 0, sizeof(m_szCurDeviceAttrId));
}

CDeviceIfAD::~CDeviceIfAD(void)
{		
	Terminate();
}

bool CDeviceIfAD::Terminate()
{
	TRACE(CDeviceIfBase::Terminate());
	return true;
}

/******************************************************************************/
/* Function Name  : Initialize                                                */
/* Description    : 接口初始化                                                */
/* Input          : int iChannelId         通道号                             */
/*                  TKU32 unHandleCtrl     动作队列内存句柄                   */
/* Output         : none                                                      */
/* Return         : none                                                      */
/******************************************************************************/
int CDeviceIfAD::Initialize(int iChannelId, TKU32 unHandleCtrl)
{
	int ret = SYS_STATUS_FAILED;
	if(SYS_STATUS_SUCCESS != (ret = CDeviceIfBase::Initialize(iChannelId, unHandleCtrl))) 
		return ret;

	ret = SYS_STATUS_SUCCESS;
	return ret; 
}

/******************************************************************************/
/* Function Name  : PollProc                                                  */
/* Description    : 轮巡执行函数		                                      */
/* Input          : PDEVICE_INFO_POLL device_node	            轮巡结点      */
/* Output         : none													  */
/* Return         : int ret          0:成功，1:设备无响应                     */
/******************************************************************************/
int CDeviceIfAD::PollProc(PDEVICE_INFO_POLL device_node, unsigned char* respBuf, int& nRespLen)
{
	int ret = SYS_STATUS_FAILED;
	nRespLen = 0;

	int channelid = atoi(device_node->Upper_Channel);
	do 
	{
		switch(channelid)
		{	
			//电流模拟量输入
		case INTERFACE_AD_A0:
		case INTERFACE_AD_A1:
		case INTERFACE_AD_A2:
		case INTERFACE_AD_A3:
			//电压模拟量输入
		case INTERFACE_AD_B0:
		case INTERFACE_AD_B1:
		case INTERFACE_AD_B2:
		case INTERFACE_AD_B3:
			{
				float adValue = 0.0;
				HardIF_BackLightSet(channelid, 1);
				if(SYS_STATUS_SUCCESS == HardIF_GetADValue(channelid, adValue))
				{
					nRespLen = sizeof(float);
					memcpy(respBuf, (unsigned char*)&adValue, nRespLen);
					ret = SYS_STATUS_SUCCESS;
				}
				HardIF_BackLightSet(channelid, 0);
			}
			break;
			//开关量输入A组
		case INTERFACE_DIN_A0:
		case INTERFACE_DIN_A1:
		case INTERFACE_DIN_A2:
		case INTERFACE_DIN_A3:
			//开关量输入B组
		case INTERFACE_DIN_B0:
		case INTERFACE_DIN_B1:
		case INTERFACE_DIN_B2:
		case INTERFACE_DIN_B3:
			{
				float adValue = 0;
				HardIF_BackLightSet(channelid, 1);
				if(SYS_STATUS_SUCCESS == HardIF_GetADValue(channelid, adValue))
				{
					unsigned char diValue = (unsigned char)adValue;
					nRespLen = sizeof(unsigned char);
					memcpy(respBuf, (unsigned char*)&diValue, nRespLen);
					ret = SYS_STATUS_SUCCESS;
				}	
				HardIF_BackLightSet(channelid, 0);	
			}
			break;
		case INTERFACE_DOUT_A1:
		case INTERFACE_DOUT_A2:
		case INTERFACE_DOUT_A3:
		case INTERFACE_DOUT_A4:
		case INTERFACE_DOUT_B1:
		case INTERFACE_DOUT_B2:
		case INTERFACE_DOUT_B3:
		case INTERFACE_DOUT_B4:
		case INTERFACE_AV_OUT_1:
			{
				float adValue = 0;
				if(SYS_STATUS_SUCCESS == HardIF_GetADValue(channelid, adValue))
				{
					unsigned char diValue = (unsigned char)adValue;
					nRespLen = sizeof(unsigned char);
					memcpy(respBuf, (unsigned char*)&diValue, nRespLen);
					ret = SYS_STATUS_SUCCESS;
				}
			}
			break;
		}
	} while (false);
	return ret;
}

//动作执行函数
int CDeviceIfAD::ActionProc(PDEVICE_INFO_ACT device_node, ACTION_MSG& actionMsg)
{
	int ret = SYS_STATUS_FAILED;
	int nLen = 0;
	BYTE reqBuf[MAX_PACKAGE_LEN_TERMINAL] = {0};
	int channelid = atoi(device_node->Upper_Channel);

	char szData[MAX_PACKAGE_LEN_TERMINAL] = {0};
	memcpy(szData, actionMsg.ActionValue, sizeof(actionMsg.ActionValue));
	DBG(("CDeviceIfAD:ActionProc 11111\n"));
	//亮灯
	HardIF_BackLightSet(channelid, 1);
	do 
	{
		nLen = LabelIF_EncodeFormat(device_node->W_Format, NULL, NULL, szData, reqBuf, sizeof(reqBuf));
		if (nLen > 0)
		{
			HardIF_SetADValue(channelid, reqBuf[0]);
			ret = SYS_STATUS_SUCCESS;			
		}
	} while (false);
	DBG(("CDeviceIfAD:ActionProc 222\n"));
	HardIF_BackLightSet(channelid, 0);

	return ret;
}

/*************************************  END OF FILE *****************************************/
