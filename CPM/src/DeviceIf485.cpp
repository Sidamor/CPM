/******************************************************************************/
/*    File    : DeviceIf485.cpp                                               */
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
#include "Init.h"
#include "SqlCtl.h"

#include "DeviceIf485.h"
#include "HardIF.h"
#include "LabelIF.h"



/******************************************************************************/
/*                       局部宏定义                                           */
/******************************************************************************/

/******************************************************************************/
/*                       局部函数                                             */
/******************************************************************************/

CDeviceIf485::CDeviceIf485()
{
	memset(m_szCurDeviceAttrId, 0, sizeof(m_szCurDeviceAttrId));
}

CDeviceIf485::~CDeviceIf485(void)
{		
	Terminate();
}

bool CDeviceIf485::Terminate()
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
int CDeviceIf485::Initialize(int iChannelId, TKU32 unHandleCtrl)
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

int CDeviceIf485::PollProc(PDEVICE_INFO_POLL device_node, unsigned char* respBuf, int& nRespLen)
{
	int ret = SYS_STATUS_FAILED;
	BYTE reqBuf[MAX_PACKAGE_LEN_TERMINAL] = {0};
	int nLen = 0;

	DBG(("CDeviceIf485::PollProc Line82 \n"));
	//int iSendLen = 0;
	//int iNeedLen = nRespLen;
	nRespLen = 0;
	/******************************************************************************/
	//闪灯
	HardIF_BackLightSet(m_iChannelId, 1);
	do 
	{
		if (0 >= strlen(device_node->R_Format))
			break;

		//组织数据
		nLen = LabelIF_EncodeFormat(device_node->R_Format,device_node->Self_Id, NULL, NULL, reqBuf, sizeof(reqBuf));
		if (0 >= nLen)
			break;

		CHardIF_485* hardIF_485 = new CHardIF_485(m_iChannelId, device_node->BaudRate, device_node->databyte, device_node->checkMode[0], device_node->stopbyte, device_node->timeOut, 0);
		ret = hardIF_485->Exec485(respBuf, nRespLen, reqBuf, nLen);
		delete hardIF_485;
		if (0 < nRespLen && MAX_PACKAGE_LEN_TERMINAL > nRespLen)
		{
			ret = SYS_STATUS_SUCCESS;
		}
		else
		{
			ret = SYS_STATUS_DEVICE_COMMUNICATION_ERROR;
		}
	} while (false);

	HardIF_BackLightSet(m_iChannelId, 0);
	/******************************************************************************/
	return ret;
}

//动作执行函数
int CDeviceIf485::ActionProc(PDEVICE_INFO_ACT device_node, ACTION_MSG& actionMsg)
{
	int ret = SYS_STATUS_FAILED;//提交成功
	BYTE reqBuf[MAX_PACKAGE_LEN_TERMINAL] = {0};
	BYTE respBuf[MAX_PACKAGE_LEN_TERMINAL] = {0};

	int nLen = 0;
	int nRespLen = 0;
//	int iSendLen = 0;
	int channelid = atoi(device_node->Upper_Channel);

	char szData[256] = {0};
	memcpy(szData, actionMsg.ActionValue, sizeof(szData));
	DBG(("ActionProc channelid:[%d] ID[%s] szData[%s]\n", channelid, device_node->Id, szData));


	//亮灯
	HardIF_BackLightSet(channelid, 1);
	do 
	{
		if (0 >= strlen(device_node->W_Format))
		{
			ret = SYS_STATUS_ACTION_WFORMAT_ERROR;
			break;
		}
		//组织数据
		nLen = LabelIF_EncodeFormat(device_node->W_Format,device_node->Self_Id, NULL, szData, reqBuf, sizeof(reqBuf));
		if (0 >= nLen)
		{
			ret = SYS_STATUS_ACTION_ENCODE_ERROR;
			break;
		}

		CHardIF_485* hardIF_485 = new CHardIF_485(channelid, device_node->BaudRate, device_node->databyte, device_node->checkMode[0], device_node->stopbyte, device_node->timeOut, 1);
		ret = hardIF_485->Exec485(respBuf, nRespLen, reqBuf, nLen);
		delete hardIF_485;

		if(strlen(device_node->WD_Format) <= 0)//数据格式无需校验，直接返回提交成功
			break;

		if (0 < nRespLen && MAX_PACKAGE_LEN_TERMINAL > nRespLen)
		{
			ret = LabelIF_DecodeActionResponse(device_node, respBuf, nRespLen);
		}

		
	} while (false);


	HardIF_BackLightSet(channelid, 0);

	return ret;
}
/*************************************  END OF FILE *****************************************/
