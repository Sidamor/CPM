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
#include <stdio.h>
#include <error.h>
#include "Shared.h"
#include "Init.h"
#include "LabelIF.h"
#include "Util.h"
#include "DevInfoContainer.h"

int g_QueryIndex[25] = 
{
	//模拟量输入A组
	INTERFACE_AD_A0,
	INTERFACE_AD_A1,
	INTERFACE_AD_A2,
	INTERFACE_AD_A3,

	//模拟量输入B组
	INTERFACE_AD_B0,
	INTERFACE_AD_B1,
	INTERFACE_AD_B2,
	INTERFACE_AD_B3,

	//开关量输入A组
	INTERFACE_DIN_A0,
	INTERFACE_DIN_A1,
	INTERFACE_DIN_A2,
	INTERFACE_DIN_A3,         //20

	//开关量输入B组
	INTERFACE_DIN_B0,
	INTERFACE_DIN_B1,
	INTERFACE_DIN_B2,
	INTERFACE_DIN_B3,

	//开关量输出上排
	INTERFACE_DOUT_A2,
	INTERFACE_DOUT_A4,
	INTERFACE_DOUT_B2,
	INTERFACE_DOUT_B4,

	//开关量输出下排
	INTERFACE_DOUT_A1,
	INTERFACE_DOUT_A3,
	INTERFACE_DOUT_B1,
	INTERFACE_DOUT_B3,

	//继电器电源输出
	INTERFACE_AV_OUT_1
};

CDevInfoContainer::CDevInfoContainer()
{
	memset((unsigned char*)&m_ParaInfo, 0, sizeof(m_ParaInfo));
}

CDevInfoContainer::~CDevInfoContainer(void)
{		
	Terminate();
}
void CDevInfoContainer::Terminate()
{

}
bool CDevInfoContainer::Initialize()
{
    DBG(("CDevInfoContainer Initialize Begin... \n"));

	//初始化单位信息
	InitDataType();

	m_HashTableDevice = new HashedDeviceDetail();
	if (NULL == m_HashTableDevice)
	{
		return false;
	}

	m_HashTableMode = new HashedDeviceMode();
	if (NULL == m_HashTableMode)
	{
		return false;
	}

    m_HashTableRoll = new HashedDeviceRoll();
    if (NULL == m_HashTableRoll)
    {
        return false;
    }

	m_HashTableAction = new HashedDeviceAction();
	if (NULL == m_HashTableAction)
	{
		return false;
	}

	//初始化设备信息
	if(!InitDeviceInfo())
		return false;
	
	//PrintPollHash();
	DBG(("CDevInfoContainer::Initialize success!\n "));

	return true; 
}

/******************************************************************************/
/* Function Name  : InitDataType                                            */
/* Description    : 采集参数定义表                                              */
/* Input          : none                                                      */
/* Output         : none                                                      */
/* Return         : none                                                      */
/******************************************************************************/
bool CDevInfoContainer::InitDataType()
{
	const char* column = "select id, cname, unit, memo from data_type;";

	if(!InitDataTypeProc(column))
		return false;

	return true;
}

/******************************************************************************/
/* Function Name  : InitDataTypeProc                                       */
/* Description    :                                       */
/* Input          : none                                                      */
/* Output         : none                                                      */
/* Return         : none                                                      */
/******************************************************************************/
bool CDevInfoContainer::InitDataTypeProc(const char* sql)
{
	//查询数据库， 获取设备信息
	bool ret = false;	
	if(0 == g_SqlCtl.select_from_tableEx((char*)sql, CDevInfoContainer::InitDataTypeCallBack, (void *)this))
	{
		ret = true;
	}
	return ret;
}

int CDevInfoContainer::InitDataTypeCallBack(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{
	CDevInfoContainer* _this = (CDevInfoContainer*)data;
	_this->InitDataTypeCallBackProc(n_column, column_value, column_name);	
	return 0;
}
int CDevInfoContainer::InitDataTypeCallBackProc(int n_column, char **column_value, char **column_name)   //轮询表初始化
{
	PARA_INFO stParaInfo;
	memset((unsigned char*)&stParaInfo, 0, sizeof(PARA_INFO));
	
	memcpy(stParaInfo.szId, column_value[0], 2);		        //参数编号
	strcpy(stParaInfo.szCname, column_value[1]);				//参数名称
	strcpy(stParaInfo.szUnit, column_value[2]);		     		//参数单位
	strcpy(stParaInfo.szMemo, column_value[3]);		     		//备注
	memcpy((unsigned char*)(&m_ParaInfo[atoi(stParaInfo.szId)]), (unsigned char*)&stParaInfo, sizeof(PARA_INFO));
	DBG(("InitDataTypeCallBackProc: %s %s %s %s\n", stParaInfo.szId, stParaInfo.szCname, stParaInfo.szUnit, stParaInfo.szMemo));
	return 0;
}

int CDevInfoContainer::GetParaUnit(int paraType, char* outBuf)
{
	int ret = SYS_STATUS_FAILED;
	if (paraType <= 99)
	{
		strcpy(outBuf, m_ParaInfo[paraType].szUnit);
		ret = SYS_STATUS_SUCCESS;
	}
	return ret;
}

/******************************************************************************/
/* Function Name  : InitDeviceInfo                                            */
/* Description    : 设备表初始化                                              */
/* Input          : none                                                      */
/* Output         : none                                                      */
/* Return         : none                                                      */
/******************************************************************************/
bool CDevInfoContainer::InitDeviceInfo()
{
	const char* format = "select a.id, "
					" a.cname, "
					" a.status,"
					" a.self_id,"
					" a.brief,"
					" b.mode_list,"
					" a.Upper_Channel, "
					" a.Private_Attr, "
					" a.Upper "
					" from device_detail a, device_info b "
					" where substr(a.id,1,%d) = b.id;";
	char column[1024] = {0};
	sprintf(column, format, DEVICE_TYPE_LEN);
	if(!InitDeviceDetailInfo(column))
		return false;

	//查询数据库， 获取设备信息
	format = "select device_roll.id, "
					"device_roll.sn, "
					"device_detail.Upper_Channel, "
					"device_detail.Self_Id,"
					"device_roll.Status,"//V10.0.0.18
					"device_attr.R_Format,"
					"device_attr.CK_Format,"
					"device_attr.RD_Format,"
					"device_attr.RD_Comparison_Table,"
					"device_roll.defence, "
					"device_detail.Private_Attr,"
					"device_roll.Private_Attr, "
					"device_roll.v_id, "
					"device_roll.v_sn, "
					"device_roll.sta_show, "
					"device_roll.S_DEFINE, "
					"device_roll.standard, "
					"device_attr.ledger, "
					"device_roll.VALUE_LIMIT, "
					"device_roll.Attr_Name, "//V10.0.0.18
					"device_detail.STATUS, "//V10.0.0.18,用于离线判断
					"device_attr.card, "//V1.0.2.1,用于判断是否要转换为对应用户编号
					"device_detail.upper, "//V1.0.2.2,lsd设备上级编号ID
					"device_attr.value_limit, "//网元状态定义
					"device_info.private_attr "
					" from device_roll, device_detail, device_attr, device_info"
					" where device_roll.id = device_detail.id "
					" and substr(device_roll.id,1,%d) = device_attr.id"
					" and device_roll.sn = device_attr.sn"
					" and substr(device_roll.id,1,%d) = device_info.id"//V10.0.0.18
					" group by device_roll.id, device_roll.sn;";
	memset(column, 0, sizeof(column));
	sprintf(column, format, DEVICE_TYPE_LEN, DEVICE_TYPE_LEN);
	if(!InitDeviceRollInfo(column))
		return false;

	format = "select device_act.id, "
					"device_act.sn, "
					"device_detail.Upper_Channel,"
					"device_detail.Self_Id,"
					"device_act.Status,"//V10.0.0.18
					"device_cmd.W_Format,"
					"device_cmd.CK_Format,"
					"device_cmd.WD_Format,"
					"device_cmd.WD_Comparison_Table,"
					"device_detail.Private_Attr,"
					"device_act.cmdName, "//V10.0.0.18
					"device_info.ctype2, "//V10.0.0.18
					"device_act.V_ID, "//V10.0.1.4
					"device_act.V_SN, "//V10.0.1.4
					"device_cmd.flag2,"
					"device_cmd.flag3, "
					"device_detail.upper "
					"from device_detail, device_info, device_cmd, device_act "
					"where substr(device_act.id,1,%d) = device_info.id "
					"and device_act.id = device_detail.id "
					"and substr(device_act.id,1,%d) = device_cmd.id "
					"and device_act.sn = device_cmd.sn "
					"group by device_act.sn, device_act.id;";
	memset(column, 0, sizeof(column));
	sprintf(column, format, DEVICE_TYPE_LEN, DEVICE_TYPE_LEN);
	if(!InitDeviceActionInfo(column))
		return false;
	return true;
}

/******************************************************************************/
/* Function Name  : InitDeviceDetailInfo                                       */
/* Description    : 设备采集表初始化                                      */
/* Input          : none                                                      */
/* Output         : none                                                      */
/* Return         : none                                                      */
/******************************************************************************/
bool CDevInfoContainer::InitDeviceDetailInfo(char* sql)
{
	//查询数据库， 获取设备信息
	bool ret = false;	
	if(0 == g_SqlCtl.select_from_tableEx((char*)sql, CDevInfoContainer::InitDeviceDetailCallBack, (void *)this))
	{
		ret = true;
	}
	return ret;
}

int CDevInfoContainer::InitDeviceDetailCallBack(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{
	CDevInfoContainer* _this = (CDevInfoContainer*)data;
	_this->InitDeviceDetailCallBackProc(n_column, column_value, column_name);	
	return 0;
}
int CDevInfoContainer::InitDeviceDetailCallBackProc(int n_column, char **column_value, char **column_name)   //轮询表初始化
{
	DEVICE_DETAIL_INFO hash_node;
	memset(&hash_node, 0, sizeof(DEVICE_DETAIL_INFO));

	memcpy(hash_node.Id, column_value[0], DEVICE_ID_LEN);		        //设备编号
	strcpy(hash_node.szCName, column_value[1]);							//设备名
	hash_node.cStatus = atoi(column_value[2]);
	strcpy(hash_node.szSelfId, column_value[3]);
	strcpy(hash_node.szBreaf, column_value[4]);
	strcpy(hash_node.szModList, column_value[5]);						//模式列表
	hash_node.iChannel = atoi(column_value[6]);							//子通道号
	define_sscanf(column_value[7], (char*)"login_pwd=", hash_node.szPwd);		//设备密码
	memcpy(hash_node.Upper, column_value[8], DEVICE_ID_LEN);		    //上级设备编号
	DBG(("InitDeviceDetailCallBackProc %s %s %s %s %s %s %s %s %s\n", column_value[0],column_value[1],column_value[2],column_value[3],column_value[4],column_value[5], column_value[6], column_value[7], hash_node.szPwd));
	InsertDeviceDetailNode(hash_node);
	InitDeviceModeNode(hash_node.Id, hash_node.szModList);
	return 0;
}

int CDevInfoContainer::GetDeviceDetailNode(DEVICE_DETAIL_INFO& new_node)
{
	int ret = SYS_STATUS_FAILED;
	CTblLockDevice.lock();
	HashedDeviceDetail::iterator it;

	it = m_HashTableDevice->find(new_node.Id);	
	if (it != m_HashTableDevice->end())
	{
		PDEVICE_DETAIL_INFO pNode = (PDEVICE_DETAIL_INFO)(&it->second);
		memcpy((unsigned char*)&new_node, (unsigned char*)pNode, sizeof(DEVICE_DETAIL_INFO));
		ret = SYS_STATUS_SUCCESS;
	}
	CTblLockDevice.unlock();
	return ret;
}
int CDevInfoContainer::InsertDeviceDetailNode(DEVICE_DETAIL_INFO& new_node)  
{
	CTblLockDevice.lock();
	HashedDeviceDetail::iterator it;

	it = m_HashTableDevice->find(new_node.Id);	
	if (it != m_HashTableDevice->end())
	{
		m_HashTableDevice->erase(it);
	}
	if(50 <= new_node.iChannel)
		new_node.cStatus = DEVICE_OFFLINE;

	m_HashTableDevice->insert(HashedDeviceDetail::value_type(new_node.Id,new_node));
	CTblLockDevice.unlock();

	return SYS_STATUS_SUCCESS;
}
int CDevInfoContainer::UpdateDeviceDetailNodeCurMode(DEVICE_DETAIL_INFO new_node)  
{
	int ret = SYS_STATUS_FAILED;
	
	DBG(("UpdateDeviceDetailNodeCurMode ...deviceId[%s] szCurrentMode[%s]\n", new_node.Id, new_node.szCurrentMode));
	CTblLockDevice.lock();
	HashedDeviceDetail::iterator it;
	it = m_HashTableDevice->find(new_node.Id);	
	if (it != m_HashTableDevice->end())
	{
		PDEVICE_DETAIL_INFO pNode = (PDEVICE_DETAIL_INFO)(&it->second);
		strcpy(pNode->szCurrentMode, new_node.szCurrentMode);
		ret = SYS_STATUS_SUCCESS;
	}
	CTblLockDevice.unlock();
	return ret;
}
int CDevInfoContainer::UpdateDeviceDetailNode(DEVICE_DETAIL_INFO& new_node)  
{
	CTblLockDevice.lock();
	HashedDeviceDetail::iterator it;

	it = m_HashTableDevice->find(new_node.Id);	
	if (it != m_HashTableDevice->end())
	{
		PDEVICE_DETAIL_INFO pNode = (PDEVICE_DETAIL_INFO)(&it->second);
		new_node.cStatus = pNode->cStatus;
		memcpy(new_node.szCurrentMode, pNode->szCurrentMode, DEVICE_ACTION_SN_LEN_TEMP);
		m_HashTableDevice->erase(it);
	}
	DEVICE_DETAIL_INFO temp_node;
	memcpy((unsigned char*)&temp_node, (unsigned char*)&new_node, sizeof(DEVICE_DETAIL_INFO));
	m_HashTableDevice->insert(HashedDeviceDetail::value_type(temp_node.Id,temp_node));
	CTblLockDevice.unlock();
	return SYS_STATUS_SUCCESS;
}
int CDevInfoContainer::DeleteDeviceDetailNode(DEVICE_DETAIL_INFO new_node)  
{
	CTblLockDevice.lock();
	HashedDeviceDetail::iterator it;

	it = m_HashTableDevice->find(new_node.Id);	
	if (it != m_HashTableDevice->end())
	{
		m_HashTableDevice->erase(it);
	}
	CTblLockDevice.unlock();
	return SYS_STATUS_SUCCESS;
}

//初始化模式表
int CDevInfoContainer::InitDeviceModeNode(char* deviceId, char* modeList)  
{
	DEVICE_MODE_INFO stDeviceMode;
	char* savePtr = NULL;
    char* split_result = strtok_r(modeList, ";", &savePtr);
	while(NULL != split_result)
	{
		do
		{
			if(0 == strncmp(split_result, ACTION_DEFENCE, DEVICE_MODE_LEN))//过滤布防/撤防模式
				break;
			char* pModeName = strstr(split_result, ",");
			if(NULL != pModeName)
			{
				memset((unsigned char*)&stDeviceMode, 0, sizeof(DEVICE_MODE_INFO));
				memcpy(stDeviceMode.Id, deviceId, DEVICE_ID_LEN);
				memcpy(stDeviceMode.Id + DEVICE_ID_LEN, split_result, DEVICE_MODE_LEN);
				pModeName += 1;
				strcpy(stDeviceMode.szModeName, pModeName);
				
				DBG(("ModeId: [%s] ModeName[%s]\n", stDeviceMode.Id, pModeName));	
				InsertDeviceModeNode(stDeviceMode);
			}
		}while(false);
		split_result = strtok_r(NULL, ";", &savePtr);
	}
	return SYS_STATUS_SUCCESS;
}
//添加模式记录到模式记录表 lhy
int CDevInfoContainer::GetDeviceModeNode(DEVICE_MODE_INFO& new_node)  
{
	int ret = SYS_STATUS_FAILED;
	CTblLockMode.lock();
	HashedDeviceMode::iterator it;
	PDEVICE_MODE_INFO pstModeInfo = NULL;
	it = m_HashTableMode->find(new_node.Id);	
	if (it != m_HashTableMode->end())
	{
		pstModeInfo = (PDEVICE_MODE_INFO)(&it->second);
		memcpy((unsigned char*)&new_node, (unsigned char*)pstModeInfo, sizeof(DEVICE_MODE_INFO));
		ret = SYS_STATUS_SUCCESS;
	}
	CTblLockMode.unlock();
	return ret;
}

//添加模式记录到模式记录表 lhy
int CDevInfoContainer::InsertDeviceModeNode(DEVICE_MODE_INFO new_node)  
{
	CTblLockMode.lock();
	HashedDeviceMode::iterator it;

	it = m_HashTableMode->find(new_node.Id);	
	if (it != m_HashTableMode->end())
	{
		m_HashTableMode->erase(it);
	}
	m_HashTableMode->insert(HashedDeviceMode::value_type(new_node.Id,new_node));
	CTblLockMode.unlock();

	return SYS_STATUS_SUCCESS;
}

int CDevInfoContainer::InitDeviceModeNodeCurValue(char* deviceId, char* actionId, char* actionName, char flag, char flag2)  
{	
	if(0 == strcmp(actionId, ACTION_SET_DEFENCE) || 0 == strcmp(actionId, ACTION_REMOVE_DEFENCE))
		return SYS_STATUS_SUCCESS;
	//更新设备表中的当前模式
	DEVICE_DETAIL_INFO stDeviceDetail;
	memset((unsigned char*)&stDeviceDetail, 0, sizeof(DEVICE_DETAIL_INFO));
	memcpy(stDeviceDetail.Id, deviceId, DEVICE_ID_LEN);


	if(SYS_STATUS_SUCCESS != GetDeviceDetailNode(stDeviceDetail))
		return SYS_STATUS_FAILED;

	if('1' == flag && '1' == flag2 && (NULL == stDeviceDetail.szCurrentMode || 0 >= strlen(stDeviceDetail.szCurrentMode)))
	{
		DBG(("InitDeviceModeNodeCurValue ...deviceId[%s] actionId[%s] flag[%c]\n", deviceId, actionId, flag));
		strcat(stDeviceDetail.szCurrentMode, actionId);
		UpdateDeviceDetailNodeCurMode(stDeviceDetail);
	}

	//更新模式表中的模式当前值
	DEVICE_MODE_INFO stDeviceMode;
	memset((unsigned char*)&stDeviceMode, 0, sizeof(DEVICE_MODE_INFO));
	memcpy(stDeviceMode.Id, deviceId, DEVICE_ID_LEN);
	memcpy(stDeviceMode.Id + DEVICE_ID_LEN, actionId, DEVICE_MODE_LEN);
	CTblLockMode.lock();
	HashedDeviceMode::iterator it;
	it = m_HashTableMode->find(stDeviceMode.Id);	
	if (it != m_HashTableMode->end())
	{
		PDEVICE_MODE_INFO pstModeInfo = (PDEVICE_MODE_INFO)(&it->second);
		if('1' == flag && '1' == flag2 && 0 >= strlen(pstModeInfo->curActionId))
		{
			memcpy(pstModeInfo->curActionId, actionId + DEVICE_MODE_LEN, DEVICE_CUR_ACTION_LEN);
			strcpy(pstModeInfo->szActionName, actionName);
			DBG(("InitDeviceModeNodeCurValue id[%s] curActionId[%s] szModeName[%s] szActionName[%s]5555\n", pstModeInfo->Id, pstModeInfo->curActionId, pstModeInfo->szModeName, pstModeInfo->szActionName));
		}
	}
	CTblLockMode.unlock();
	return SYS_STATUS_SUCCESS;
}

int CDevInfoContainer::UpdateDeviceDetailMode(char* deviceId, char* actionId)
{
	if(0 == strcmp(actionId, ACTION_SET_DEFENCE) || 0 == strcmp(actionId, ACTION_REMOVE_DEFENCE))
		return SYS_STATUS_SUCCESS;
	DBG(("UpdateDeviceDetailMode deviceId[%s] actionId[%s]\n", deviceId, actionId));
	//更新设备表中的当前模式
	DEVICE_DETAIL_INFO stDeviceDetail;
	memset((unsigned char*)&stDeviceDetail, 0, sizeof(DEVICE_DETAIL_INFO));
	memcpy(stDeviceDetail.Id, deviceId, DEVICE_ID_LEN);
	strcat(stDeviceDetail.szCurrentMode, actionId);
	UpdateDeviceDetailNodeCurMode(stDeviceDetail);

	return SYS_STATUS_SUCCESS;
}
int CDevInfoContainer::UpdateDeviceModeNodeCurValue(char* deviceId, char* actionId, char* actionName)
{
	if(0 == strcmp(actionId, ACTION_SET_DEFENCE) || 0 == strcmp(actionId, ACTION_REMOVE_DEFENCE))
		return SYS_STATUS_SUCCESS;
	DBG(("UpdateDeviceModeNodeCurValue deviceId[%s] actionId[%s]\n", deviceId, actionId));
	
	//更新模式表中的模式当前值
	DEVICE_MODE_INFO stDeviceMode;
	memset((unsigned char*)&stDeviceMode, 0, sizeof(DEVICE_MODE_INFO));
	memcpy(stDeviceMode.Id, deviceId, DEVICE_ID_LEN);
	memcpy(stDeviceMode.Id + DEVICE_ID_LEN, actionId, DEVICE_MODE_LEN);
	CTblLockMode.lock();
	HashedDeviceMode::iterator it;
	
	it = m_HashTableMode->find(stDeviceMode.Id);
	if (it != m_HashTableMode->end())
	{		
		PDEVICE_MODE_INFO pstModeInfo = (PDEVICE_MODE_INFO)(&it->second);
		memcpy(pstModeInfo->curActionId, actionId + DEVICE_MODE_LEN, DEVICE_CUR_ACTION_LEN);
		strcpy(pstModeInfo->szActionName, actionName);
		DBG(("UpdateDeviceModeNodeCurValue id[%s] curActionId[%s] szModeName[%s] szActionName[%s]5555\n", pstModeInfo->Id, pstModeInfo->curActionId, pstModeInfo->szModeName, pstModeInfo->szActionName));
	}
	CTblLockMode.unlock();
	return SYS_STATUS_SUCCESS;
}

int CDevInfoContainer::UpdateDeviceModeNode(DEVICE_MODE_INFO new_node)  
{
	CTblLockMode.lock();
	HashedDeviceMode::iterator it;

	it = m_HashTableMode->find(new_node.Id);	
	if (it != m_HashTableMode->end())
	{
		m_HashTableMode->erase(it);
	}
	m_HashTableMode->insert(HashedDeviceMode::value_type(new_node.Id,new_node));
	CTblLockMode.unlock();
	return SYS_STATUS_SUCCESS;
}

int CDevInfoContainer::DeleteDeviceModeNode(DEVICE_MODE_INFO new_node)  
{
	CTblLockMode.lock();
	HashedDeviceMode::iterator it;

	it = m_HashTableMode->find(new_node.Id);	
	if (it != m_HashTableMode->end())
	{
		m_HashTableMode->erase(it);
	}
	CTblLockMode.unlock();
	//删除模式记录表中的记录
	return SYS_STATUS_SUCCESS;
}

/******************************************************************************/
/* Function Name  : ModeHashDeleteByDevice                                   */
/* Description    : 删除模式设备表节点                                        */
/* Input          : NET_DEVICE_INFO_UPDATE new_node      设备节点             */
/* Output         : none                                                      */
/* Return         : true   节点更新                                           */
/*                  false  节点增加                                           */
/******************************************************************************/
int CDevInfoContainer::ModeHashDeleteByDevice(char* deviceId)
{
	int ret = SYS_STATUS_FAILED;
	HashedDeviceMode::iterator it;
	PDEVICE_MODE_INFO device_node;
	DEVICE_MODE_INFO startNode[256];
	int startNodeCount = 0;

	CTblLockMode.lock();
	for (it = m_HashTableMode->begin(); it != m_HashTableMode->end(); it++)
	{
		device_node = (PDEVICE_MODE_INFO)(&it->second);
		if (0 == strncmp(device_node->Id, deviceId, DEVICE_ID_LEN))
		{
			memcpy((unsigned char*)&startNode[startNodeCount++], (unsigned char*)device_node, sizeof(DEVICE_MODE_INFO));
		}
	}
	for(int i=0; i<startNodeCount; i++)
	{
		it = m_HashTableMode->find(startNode[i].Id);	
		if (it != m_HashTableMode->end())
		{
			m_HashTableMode->erase(it);
		}
	}
	CTblLockMode.unlock();
	ret = SYS_STATUS_SUCCESS;
	return ret;
}

/******************************************************************************/
/* Function Name  : InitDeviceRollInfo                                       */
/* Description    : 设备采集表初始化                                      */
/* Input          : none                                                      */
/* Output         : none                                                      */
/* Return         : none                                                      */
/******************************************************************************/
bool CDevInfoContainer::InitDeviceRollInfo(char* sql)
{
	//查询数据库， 获取设备信息
	bool ret = false;	
	if(0 == g_SqlCtl.select_from_tableEx((char*)sql, CDevInfoContainer::InitDeviceRollCallBack, (void *)this))
	{
		ret = true;
	}
	return ret;
}

int CDevInfoContainer::InitDeviceRollCallBack(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{
	CDevInfoContainer* _this = (CDevInfoContainer*)data;
	_this->InitDeviceRollCallBackProc(n_column, column_value, column_name);	
	return 0;
}
int CDevInfoContainer::InitDeviceRollCallBackProc(int n_column, char **column_value, char **column_name)   //轮询表初始化
{
	DEVICE_INFO_POLL hash_node;
	memset(&hash_node, 0, sizeof(DEVICE_INFO_POLL));

	memcpy(hash_node.Id, column_value[0], DEVICE_ID_LEN);		        //设备编号
	memcpy(hash_node.Id+DEVICE_ID_LEN, column_value[1], DEVICE_SN_LEN); //指令SN

	strcpy(hash_node.Upper_Channel, column_value[2]);					//通道编号
	strcpy(hash_node.Self_Id, column_value[3]);		     		        //子ID号

	hash_node.cBeOn = column_value[4][0];								//启用状态

	strcpy(hash_node.R_Format, column_value[5]);						//读格式
	strcpy(hash_node.Rck_Format, column_value[6]);                      //读验证
	strcpy(hash_node.RD_Format, column_value[7]);						//数据分析格式
	strcpy(hash_node.RD_Comparison_table, column_value[8]);				//数据对照表

	char szTimeSet[20] = {0};
	strcpy(szTimeSet, column_value[9]);                                 //布防时间
	LabelIF_GetDefenceDate(szTimeSet, &hash_node);

	LabelIF_GetDeviceQueryPrivatePara(column_value[10], &hash_node);			//设备私有参数
	LabelIF_GetDeviceAttrQueryPrivatePara(column_value[11], &hash_node);		//设备属性私有参数

	memcpy(hash_node.V_Id, column_value[12], DEVICE_ID_LEN);		        //设备编号
	memcpy(hash_node.V_Id+DEVICE_ID_LEN, column_value[13], DEVICE_SN_LEN); //指令SN

	hash_node.cStaShow = column_value[14][0];
	strcpy(hash_node.szS_Define, column_value[15]);							//状态定义
	strcpy(hash_node.szStandard, column_value[16]);							//标准范围
	hash_node.cLedger = column_value[17][0];								//是否统计的标记

	LabelIF_GetDeviceValueLimit(column_value[18], &hash_node);			//设备私有参数

	strcpy(hash_node.szAttrName, column_value[19]);							//属性名称
	hash_node.retryCount = 0;
	char temp[64] = {0};
	strcpy(temp, column_value[20]);										//
	if( atoi(temp) > 1 )
	{
		hash_node.offLineCnt = hash_node.offLineTotle;
	}
	else if (1 == atoi(temp))
	{
		hash_node.offLineCnt = 0;
	}

	hash_node.cIsNeedIdChange = column_value[21][0];		//需要将值转换为用户编号
	strcpy(hash_node.szAttrValueLimit, column_value[23]);	//网元状态转译

	//定制库
	if (NULL != strstr(column_value[24], "soif"))
	{
		hash_node.isSOIF = 1;
		
	}
	//透明传输
	if (NULL != strstr(column_value[24], "transparent"))
	{
		hash_node.isTransparent = 1;
	}
	
	memset((unsigned char*)&hash_node.stLastValueInfo, 0, sizeof(LAST_VALUE_INFO));
	//插入前，设置相同设备相同轮巡方式的属性的pre和next结点
	memcpy(hash_node.PreId, hash_node.Id, DEVICE_ID_LEN + DEVICE_SN_LEN);
	memcpy(hash_node.NextId, hash_node.Id, DEVICE_ID_LEN + DEVICE_SN_LEN);

	//插入前，设置相同通道设备的pre和next结点
	memcpy(hash_node.ChannelPreId, hash_node.Id, DEVICE_ID_LEN + DEVICE_SN_LEN);
	memcpy(hash_node.ChannelNextId, hash_node.Id, DEVICE_ID_LEN + DEVICE_SN_LEN);

	InsertQueryNode(hash_node);
	DBG(("InitDeviceRollCallBackProc End id[%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%lu]\n",
								hash_node.Id, column_value[2], column_value[3], column_value[4], column_value[5], column_value[6], column_value[7]
								, column_value[8], column_value[9], column_value[10], column_value[11], column_value[12], column_value[13],
									column_value[14], column_value[15], column_value[16], column_value[17], column_value[18], column_value[19], hash_node.Frequence));
	return 0;
}

int CDevInfoContainer::InsertQueryNode(DEVICE_INFO_POLL& new_node)   //将结点插入到hash表公共接口
{
	int ret = SYS_STATUS_FAILED;
	CTblLockRoll.lock();
	ret = doInsertQueryNode(new_node);
	CTblLockRoll.unlock();
	return ret;
}

int CDevInfoContainer::doInsertQueryNode(DEVICE_INFO_POLL& new_node)   //将结点插入到hash表私有接口
{
	HashedDeviceRoll::iterator itQuery;
	PDEVICE_INFO_POLL device_node;
	bool bSameAttrSet = false;
	bool bSameChannelSet = false;

	char chanelPre[DEVICE_ATTR_ID_LEN_TEMP] = {0};
	char attrPre[DEVICE_ATTR_ID_LEN_TEMP] = {0};
	for (itQuery = m_HashTableRoll->begin(); itQuery != m_HashTableRoll->end(); itQuery++)
	{
		device_node = (PDEVICE_INFO_POLL)(&itQuery->second);

		if ( 0 == strncmp(device_node->Id, new_node.Id, DEVICE_ID_LEN) //设备号相同
			&& 0 == strcmp(new_node.R_Format, device_node->R_Format)//读格式相同
			&& new_node.Frequence == device_node->Frequence//采集周期相同
			&& 0 == memcmp((unsigned char*)&new_node.startTime, (unsigned char*)&device_node->startTime, sizeof(struct tm))
			&& 0 == memcmp((unsigned char*)&new_node.endTime, (unsigned char*)&device_node->endTime, sizeof(struct tm)))//布防时间相同
		{
			if (!bSameAttrSet)
			{
				strncpy(new_node.PreId, device_node->Id, DEVICE_ATTR_ID_LEN);
				strncpy(new_node.NextId, device_node->NextId, DEVICE_ATTR_ID_LEN);	

				if (0 == strncmp(device_node->PreId, device_node->Id, DEVICE_ATTR_ID_LEN))//原来只有一个结点，则更新原结点的上级结点
				{
					strncpy(device_node->PreId, new_node.Id, DEVICE_ATTR_ID_LEN);		
				}
				else
				{
					strncpy(attrPre, device_node->NextId, DEVICE_ATTR_ID_LEN);
				}
				strncpy(device_node->NextId, new_node.Id, DEVICE_ATTR_ID_LEN);		

				bSameAttrSet = true;
			}
		}
		//lhy else if (atoi(device_node->Upper_Channel) == atoi(new_node.Upper_Channel)//通道相同
		if (atoi(device_node->Upper_Channel) == atoi(new_node.Upper_Channel)//通道相同
		|| (atoi(device_node->Upper_Channel) >= INTERFACE_AD_A0 && atoi(device_node->Upper_Channel) <= INTERFACE_AV_OUT_1 
					&& atoi(new_node.Upper_Channel) >= INTERFACE_AD_A0 && atoi(new_node.Upper_Channel) <= INTERFACE_AV_OUT_1 ))//或者是开关量和继电器通道
		{
			if (!bSameChannelSet)
			{
				strncpy(new_node.ChannelPreId, device_node->Id, DEVICE_ATTR_ID_LEN);
				strncpy(new_node.ChannelNextId, device_node->ChannelNextId, DEVICE_ATTR_ID_LEN);	

				if (0 == strncmp(device_node->ChannelPreId, device_node->Id, DEVICE_ATTR_ID_LEN))//原来只有一个结点，则更新原结点的上级结点
				{
					strncpy(device_node->ChannelPreId, new_node.Id, DEVICE_ATTR_ID_LEN);		
				}
				else
				{
					strncpy(chanelPre, device_node->ChannelNextId, DEVICE_ATTR_ID_LEN);
				}
				strncpy(device_node->ChannelNextId, new_node.Id, DEVICE_ATTR_ID_LEN);		
				
				bSameChannelSet = true;
			}
		}

		if (bSameAttrSet && bSameChannelSet)
		{
			break;
		}
	}
	if (DEVICE_ATTR_ID_LEN == strlen(attrPre))
	{
		DEVICE_INFO_POLL tempNode;
		memset((unsigned char*)&tempNode, 0, sizeof(DEVICE_INFO_POLL));
		strncpy(tempNode.Id, attrPre, DEVICE_ATTR_ID_LEN);

		itQuery = m_HashTableRoll->find(tempNode.Id);	
		if (itQuery != m_HashTableRoll->end())
		{
			PDEVICE_INFO_POLL pNode = (PDEVICE_INFO_POLL)(&itQuery->second);
			strncpy(pNode->PreId, new_node.Id, DEVICE_ATTR_ID_LEN);
		}
	}

	if (DEVICE_ATTR_ID_LEN == strlen(chanelPre))
	{
		DEVICE_INFO_POLL tempNode;
		memset((unsigned char*)&tempNode, 0, sizeof(DEVICE_INFO_POLL));
		strncpy(tempNode.Id, chanelPre, DEVICE_ATTR_ID_LEN);

		itQuery = m_HashTableRoll->find(tempNode.Id);	
		if (itQuery != m_HashTableRoll->end())
		{
			PDEVICE_INFO_POLL pNode = (PDEVICE_INFO_POLL)(&itQuery->second);
			strncpy(pNode->ChannelPreId, new_node.Id, DEVICE_ATTR_ID_LEN);
		}
	}
	memset((unsigned char*)&new_node.stLastValueInfo.LastTime, 0, sizeof(struct timeval));
	
	if(atoi(new_node.Upper_Channel) >= INTERFACE_NET_INPUT)//lhy
	{
		struct timeval tmval_NowTime;
		gettimeofday(&tmval_NowTime, NULL);	
		memcpy((unsigned char*)(&(new_node.stLastValueInfo.LastTime)), (unsigned char*)(&tmval_NowTime), sizeof(struct timeval));
	}
	m_HashTableRoll->insert(HashedDeviceRoll::value_type(new_node.Id,new_node));
	DBG(("doInsertQueryNode channel[%s] id[%s] prechannelid[%s] newchannelid[%s] preattrid[%s] nextattrid[%s]\n",
						new_node.Upper_Channel, new_node.Id, new_node.ChannelPreId, new_node.ChannelNextId, new_node.PreId, new_node.NextId));
	return SYS_STATUS_SUCCESS;
}


/******************************************************************************/
/* Function Name  : DeleteQueryNode                               */
/* Description    : 删除轮巡设备表节点                                        */
/* Input          : DEVICE_INFO_POLL new_node      要删除的结点编号                       */
/* Output         : none                                                      */
/* Return         :                                            */
/*                                                             */
/******************************************************************************/
int CDevInfoContainer::DeleteQueryNode(DEVICE_INFO_POLL new_node)
{
	int ret = SYS_STATUS_FAILED;
	CTblLockRoll.lock();
	ret = doDeleteQueryNode(new_node);
	CTblLockRoll.unlock();
	return ret;
}

/******************************************************************************/
/* Function Name  : doDeleteQueryNode                               */
/* Description    : 删除轮巡设备表节点                                        */
/* Input          : DEVICE_INFO_POLL new_node      要删除的结点编号                       */
/* Output         : none                                                      */
/* Return         :                                            */
/*                                                             */
/******************************************************************************/
int CDevInfoContainer::doDeleteQueryNode(DEVICE_INFO_POLL new_node)
{
	int ret = SYS_STATUS_FAILED;
	HashedDeviceRoll::iterator it;

	DEVICE_INFO_POLL hash_node;
	DEVICE_INFO_POLL tempNode;
	memset((unsigned char*)&hash_node, 0, sizeof(DEVICE_INFO_POLL));
	strncpy(hash_node.Id, new_node.Id, DEVICE_ATTR_ID_LEN);

	it = m_HashTableRoll->find(hash_node.Id);	
	if (it != m_HashTableRoll->end())
	{
		PDEVICE_INFO_POLL pNode = (PDEVICE_INFO_POLL)(&it->second);
		memcpy((unsigned char*)&hash_node, (unsigned char*)pNode, sizeof(DEVICE_INFO_POLL));

		m_HashTableRoll->erase(it);

		if (0 != strncmp(hash_node.PreId, hash_node.Id, DEVICE_ATTR_ID_LEN))
		{
			memset((unsigned char*)&tempNode, 0, sizeof(tempNode));
			strcpy(tempNode.Id, hash_node.PreId);
			it = m_HashTableRoll->find(tempNode.Id);	
			if (it != m_HashTableRoll->end())
			{
				PDEVICE_INFO_POLL pNode = (PDEVICE_INFO_POLL)(&it->second);
				strncpy(pNode->NextId, hash_node.NextId, DEVICE_ATTR_ID_LEN);
			}
		}

		if (0 != strncmp(hash_node.NextId, hash_node.Id, DEVICE_ATTR_ID_LEN))
		{
			memset((unsigned char*)&tempNode, 0, sizeof(tempNode));
			strcpy(tempNode.Id, hash_node.NextId);
			it = m_HashTableRoll->find(tempNode.Id);	
			if (it != m_HashTableRoll->end())
			{
				PDEVICE_INFO_POLL pNode = (PDEVICE_INFO_POLL)(&it->second);
				strncpy(pNode->PreId, hash_node.PreId, DEVICE_ATTR_ID_LEN);
			}
		}

		if (0 != strncmp(hash_node.ChannelPreId, hash_node.Id, DEVICE_ATTR_ID_LEN))
		{
			memset((unsigned char*)&tempNode, 0, sizeof(tempNode));
			strncpy(tempNode.Id, hash_node.ChannelPreId, DEVICE_ATTR_ID_LEN);
			it = m_HashTableRoll->find(tempNode.Id);	
			if (it != m_HashTableRoll->end())
			{
				PDEVICE_INFO_POLL pNode = (PDEVICE_INFO_POLL)(&it->second);
				strncpy(pNode->ChannelNextId, hash_node.ChannelNextId, DEVICE_ATTR_ID_LEN);
			}
		}
		if (0 != strncmp(hash_node.ChannelNextId, hash_node.Id, DEVICE_ATTR_ID_LEN))
		{
			memset((unsigned char*)&tempNode, 0, sizeof(tempNode));
			strncpy(tempNode.Id, hash_node.ChannelNextId, DEVICE_ATTR_ID_LEN);
			it = m_HashTableRoll->find(tempNode.Id);	
			if (it != m_HashTableRoll->end())
			{
				PDEVICE_INFO_POLL pNode = (PDEVICE_INFO_POLL)(&it->second);
				strncpy(pNode->ChannelPreId, hash_node.ChannelPreId, DEVICE_ATTR_ID_LEN);
			}
		}
	}
	else
	{
		DBG(("Can not find id[%s]\n", new_node.Id));
	}
	ret = SYS_STATUS_SUCCESS;

	return ret;
}
/******************************************************************************/
/* Function Name  : InitDeviceActionInfo                                      */
/* Description    : 设备动作表初始化                                          */
/* Input          : none                                                      */
/* Output         : none                                                      */
/* Return         : none                                                      */
/******************************************************************************/
bool CDevInfoContainer::InitDeviceActionInfo(char* sql)
{
	//查询数据库， 获取设备信息
	bool ret = false;
	if(0 == g_SqlCtl.select_from_tableEx((char*)sql, CDevInfoContainer::InitDeviceActionCallBack, (void *)this))
	{
		ret = true;
	}
	return ret;
}

int CDevInfoContainer::InitDeviceActionCallBack(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{
	CDevInfoContainer* _this = (CDevInfoContainer*)data;
	_this->InitDeviceActionCallBackProc(n_column, column_value, column_name);
	return 0;
}

int CDevInfoContainer::InitDeviceActionCallBackProc(int n_column, char **column_value, char **column_name)    //动作表初始化
{
	int ret = -1;
	DEVICE_INFO_ACT hash_node;
	memset(&hash_node, 0, sizeof(DEVICE_INFO_ACT));

	memcpy(hash_node.Id, column_value[0], DEVICE_ID_LEN);		        //设备编号
	
	memcpy(hash_node.Id+DEVICE_ID_LEN, column_value[1], DEVICE_ACTION_SN_LEN); //指令SN

	strcpy(hash_node.Upper_Channel, column_value[2]);					//通道编号
	strcpy(hash_node.Self_Id, column_value[3]);		     		        //子ID号

	hash_node.cBeOn = column_value[4][0];								//启用状态

	strcpy(hash_node.W_Format, column_value[5]);						//写格式
	strcpy(hash_node.Wck_Format, column_value[6]);                      //写校验
	strcpy(hash_node.WD_Format, column_value[7]);						//数据分析格式
	strcpy(hash_node.WD_Comparison_table, column_value[8]);				//数据对照表

	LabelIF_GetDeviceActionPrivatePara(column_value[9], &hash_node);			//

	strcpy(hash_node.szCmdName, column_value[10]);				//数据对照表
	strcpy(hash_node.ctype2, column_value[11]);				//动作类型
	//hash_node.timeOut = 1000;

	if(NULL != column_value[13] && 0 < strlen(column_value[13]))
	{
		memcpy(hash_node.szVActionId, column_value[12], DEVICE_ID_LEN);
		memcpy(hash_node.szVActionId + DEVICE_ID_LEN, column_value[13], DEVICE_ACTION_SN_LEN);
	}
	hash_node.flag2 = column_value[14][0];
	hash_node.flag3 = column_value[15][0];

	strcpy(hash_node.Upper, column_value[16]);						//上级设备ID

	CLockAction.lock();
	m_HashTableAction->insert(HashedDeviceAction::value_type(hash_node.Id,hash_node));
	CLockAction.unlock();

	//更新模式表当前模式，设置默认值
	char deviceId[DEVICE_ID_LEN_TEMP] = {0};
	strncpy(deviceId, hash_node.Id, DEVICE_ID_LEN);
	char actionId[DEVICE_ACTION_SN_LEN_TEMP] = {0};
	strcpy(actionId, column_value[1]);
	InitDeviceModeNodeCurValue(deviceId, actionId, hash_node.szCmdName, hash_node.flag2, hash_node.flag3);
	return ret;
}

int CDevInfoContainer::GetDeviceStatusInfo(char* deviceId, DEVICE_DETAIL_STATUS& stPollNode)
{
	int ret = SYS_STATUS_FAILED;
	bool isDeviceExist = false;
	bool isDisarm = false;//撤防
	bool isOffline = false;//离线
	bool isException = false;//异常
	bool isMainShow = false;
	int aliasStatus = 3;		//属性显示为网元状态 图标Status代码
	char szShow[512] = {0};//

	memset((unsigned char*)&stPollNode, 0, sizeof(DEVICE_DETAIL_STATUS));

	//获取设备信息
	DBG(("GetDeviceStatusInfo: id[%s]\n", deviceId));
	HashedDeviceDetail::iterator itDetail;
	DEVICE_DETAIL_INFO stDeviceDetail;
	memset((unsigned char*)&stDeviceDetail, 0, sizeof(DEVICE_DETAIL_INFO));
	memcpy(stDeviceDetail.Id, deviceId, DEVICE_ID_LEN);
	CTblLockDevice.lock();
	itDetail = m_HashTableDevice->find(stDeviceDetail.Id);	
	if (itDetail != m_HashTableDevice->end())
	{
		PDEVICE_DETAIL_INFO pNode = (PDEVICE_DETAIL_INFO)(&itDetail->second);
		memcpy((unsigned char*)&stDeviceDetail, (unsigned char*)pNode, sizeof(DEVICE_DETAIL_INFO));
		strcpy(stPollNode.szCName, stDeviceDetail.szCName);
		isDeviceExist = true;
	}
	CTblLockDevice.unlock();
	
	if (!isDeviceExist)
	{
		return ret;
	}

	ret = SYS_STATUS_SUCCESS;
	if (DEVICE_NOT_USE == stDeviceDetail.cStatus)
	{
		isDisarm = true;
	}

	if (DEVICE_OFFLINE == stDeviceDetail.cStatus)
	{
		isOffline = true;
	}
	if (!isDisarm && !isOffline)//撤防或离线时不需要显示属性值
	{
		HashedDeviceRoll::iterator it;
		CTblLockRoll.lock();
		for (it = m_HashTableRoll->begin(); it != m_HashTableRoll->end(); it++)
		{
			PDEVICE_INFO_POLL device_node = (PDEVICE_INFO_POLL)(&it->second);
			if (0 == strncmp(device_node->Id, deviceId, DEVICE_ID_LEN) && '0' != device_node->cBeOn)//属性同属于一个设备，并且在采集
			{
				DBG(("GetDeviceStatusInfo deviceId[%s] cBeOn[%c]\n", device_node->Id, device_node->cBeOn));
				int attrStatus = DEVICE_USE;
				if (0 == strcmp(device_node->stLastValueInfo.Value, "NULL")
					|| 1 == device_node->stLastValueInfo.DataStatus)//有离线或异常
				{
					attrStatus = DEVICE_EXCEPTION;
					isException = true;
				}
				if('1' == device_node->cStaShow)//是显示属性
				{
					if (0 != strcmp(device_node->stLastValueInfo.Value, "NULL")
						&& SYS_STATUS_SUCCESS == GetShowString(device_node->stLastValueInfo.Value, device_node->szS_Define, szShow))
					{
						isMainShow = true;
						strcat(szShow, m_ParaInfo[atoi(device_node->stLastValueInfo.Type)].szUnit);
						DBG(("GetDeviceStatusInfo显示属性 [%s][%s]\n", device_node->stLastValueInfo.Value, device_node->szAttrValueLimit));
						if (DEVICE_USE == attrStatus)
						{
							char* savePtr = NULL;
							char* split_result = NULL;
							char szAlias[6] = {0};
							char attrValueLimit[65] = {0};
							strcpy(attrValueLimit, device_node->szAttrValueLimit);
							sprintf(szAlias, "%d", attrStatus);
							split_result = strtok_r(attrValueLimit, ";", &savePtr);
							while(split_result != NULL)
							{
								char* pTemp = strstr(split_result, ",");
								if (NULL == pTemp)
								{
									break;
								}
								char szTemp[32] = {0};
								strncpy(szTemp, split_result, pTemp - split_result);
								if (atoi(szTemp) == atoi(device_node->stLastValueInfo.Value))
								{
									strcpy(szAlias,pTemp + 1);
									break;
								}
								split_result = strtok_r(NULL, ";", &savePtr);
							}
							aliasStatus = atoi(szAlias);
						}
					}
				}
			}
		}
		CTblLockRoll.unlock();
	}
	strncpy(stPollNode.Id, deviceId, DEVICE_ID_LEN);

	if (isDisarm)//撤防
	{
		stPollNode.Status = DEVICE_NOT_USE;
		strcpy(stPollNode.Show, "撤防");
	}
	else if (isOffline)//离线
	{
		stPollNode.Status = DEVICE_OFFLINE;
		strcpy(stPollNode.Show, "离线");
	}else if (isException)//异常
	{
		stPollNode.Status = DEVICE_EXCEPTION;
		if (isMainShow)
		{
			strcpy(stPollNode.Show, szShow);
		}
		else
			strcpy(stPollNode.Show, "异常");

	}
	else
	{
		stPollNode.Status = DEVICE_USE;
		if (isMainShow)
		{
			strcpy(stPollNode.Show, szShow);
			stPollNode.Status = aliasStatus;
		}
		else
			strcpy(stPollNode.Show, "正常");
	}
	return ret;
}

int CDevInfoContainer::GetDeviceParasInfo(char* deviceId, DEVICE_DETAIL_PARAS& stPollNode, void* msg, int& seq)
{
	int ret = SYS_STATUS_FAILED;
	bool isDeviceExist = false;
	bool isDisarm = false;//撤防
	bool isOffline = false;//离线
	char szShow[512] = {0};//
	char szDeviceName[128] = {0};

	memset((unsigned char*)&stPollNode, 0, sizeof(DEVICE_DETAIL_PARAS));

	//获取设备信息
	DBG(("GetDeviceParasInfo: id[%s]\n", deviceId));
	HashedDeviceDetail::iterator itDetail;
	DEVICE_DETAIL_INFO stDeviceDetail;
	memset((unsigned char*)&stDeviceDetail, 0, sizeof(DEVICE_DETAIL_INFO));
	memcpy(stDeviceDetail.Id, deviceId, DEVICE_ID_LEN);
	CTblLockDevice.lock();
	itDetail = m_HashTableDevice->find(stDeviceDetail.Id);	
	if (itDetail != m_HashTableDevice->end())
	{
		PDEVICE_DETAIL_INFO pNode = (PDEVICE_DETAIL_INFO)(&itDetail->second);
		memcpy((unsigned char*)&stDeviceDetail, (unsigned char*)pNode, sizeof(DEVICE_DETAIL_INFO));
		strcpy(szDeviceName, stDeviceDetail.szCName);
		isDeviceExist = true;
	}
	CTblLockDevice.unlock();

	if (!isDeviceExist)
	{
		return ret;
	}

	ret = SYS_STATUS_SUCCESS;
	if (DEVICE_NOT_USE == stDeviceDetail.cStatus)
	{
		isDisarm = true;//已撤防
	}

	if (DEVICE_OFFLINE == stDeviceDetail.cStatus)
	{
		isOffline = true;
	}
	if (!isDisarm && !isOffline)//撤防或离线时不需要显示属性值
	{
		HashedDeviceRoll::iterator it;
		CTblLockRoll.lock();
		for (it = m_HashTableRoll->begin(); it != m_HashTableRoll->end(); it++)
		{
			memset(szShow, 0, sizeof(szShow));
			PDEVICE_INFO_POLL device_node = (PDEVICE_INFO_POLL)(&it->second);
			if (0 == strncmp(device_node->Id, deviceId, DEVICE_ID_LEN) && '0' != device_node->cBeOn)//属性同属于一个设备，并且在采集
			{
				DBG(("GetDeviceParasInfo deviceId[%s] cBeOn[%c]\n", device_node->Id, device_node->cBeOn));
		/*
		{设备属性编号，参数名，时间，数值，单位，状态，级别，描述}
		*/
				char tempValue[1024] = {0};
				GetShowString(device_node->stLastValueInfo.Value, device_node->szS_Define, szShow);
				if (0 == strncmp(szShow, "NULL", 4) || 0 == strlen(trim(szShow, strlen(szShow))))
				{
					memset(szShow, 0, sizeof(szShow));
					strcpy(szShow, "无数据");
				}
				char szTime[21] = {0};
				DBG(("device_node->stLastValueInfo.LastTime.tv_sec[%ld]\n", device_node->stLastValueInfo.LastTime.tv_sec));
				timet2chars(device_node->stLastValueInfo.LastTime.tv_sec, szTime);
				sprintf(tempValue, "{%s ,%s ,%s ,%s ,%s ,%s ,%d ,%s ,%s }",
					device_node->Id, szDeviceName, device_node->szAttrName,szTime,szShow, m_ParaInfo[atoi(device_node->stLastValueInfo.Type)].szUnit,
					device_node->stLastValueInfo.DataStatus, device_node->stLastValueInfo.szLevel, device_node->stLastValueInfo.szDescribe);

				if (strlen(tempValue) + strlen(stPollNode.ParaValues) > MAXMSGSIZE - 28)
				{
					if (0 < strlen(stPollNode.ParaValues))
					{
						char outBuf[MAXMSGSIZE] = {0};
						memcpy(outBuf, (char*)msg + MSGHDRLEN, 28);
						memcpy(outBuf+20, "0001", 4);
						strcat(outBuf, stPollNode.ParaValues);
						Net_App_Resp_Proc(msg, outBuf, strlen(outBuf), seq++);
					}
					memset(stPollNode.ParaValues, 0, sizeof(stPollNode.ParaValues));
					if (strlen(tempValue) <= MAXMSGSIZE - 28)
					{
						strcat(stPollNode.ParaValues, tempValue);
					}
				}
				else
					strcat(stPollNode.ParaValues, tempValue);
			}
		}
		CTblLockRoll.unlock();
	}
//PrintPollHash();
	DBG(("GetDeviceInfoNode: id End[%s]\n", deviceId));
	return ret;
}

int CDevInfoContainer::GetDeviceModesInfo(char* deviceId, DEVICE_DETAIL_MODES& stPollNode, void* msg, int& seq)
{
	int ret = SYS_STATUS_FAILED;
	bool isDeviceExist = false;
	bool isDisarm = false;//撤防
	bool isOffline = false;//离线
	char szCurrentMode[DEVICE_ACTION_SN_LEN_TEMP] = {0};
	memset((unsigned char*)&stPollNode, 0, sizeof(DEVICE_DETAIL_MODES));

	//获取设备当前模式信息
	DBG(("GetDeviceModesInfo: id[%s]\n", deviceId));
	HashedDeviceDetail::iterator itDetail;
	DEVICE_DETAIL_INFO stDeviceDetail;
	memset((unsigned char*)&stDeviceDetail, 0, sizeof(DEVICE_DETAIL_INFO));
	memcpy(stDeviceDetail.Id, deviceId, DEVICE_ID_LEN);
	CTblLockDevice.lock();
	itDetail = m_HashTableDevice->find(stDeviceDetail.Id);
	if (itDetail != m_HashTableDevice->end())
	{
		PDEVICE_DETAIL_INFO pNode = (PDEVICE_DETAIL_INFO)(&itDetail->second);
		memcpy((unsigned char*)&stDeviceDetail, (unsigned char*)pNode, sizeof(DEVICE_DETAIL_INFO));
		memcpy(szCurrentMode, stDeviceDetail.szCurrentMode, DEVICE_ACTION_SN_LEN);
		isDeviceExist = true;
	}
	CTblLockDevice.unlock();

	if (!isDeviceExist)
	{
		DBG(("GetDeviceModesInfo [%s] not exist\n", stDeviceDetail.Id));
		return ret;
	}

	ret = SYS_STATUS_SUCCESS;
	if (DEVICE_NOT_USE == stDeviceDetail.cStatus)
	{
		isDisarm = true;//布防
	}

	if (DEVICE_OFFLINE == stDeviceDetail.cStatus)
	{
		isOffline = true;
	}
	if (!isDisarm && !isOffline)//撤防或离线时不需要显示模式值
	{
		DEVICE_MODE_INFO stDeviceMode;
		memset((unsigned char*)&stDeviceMode, 0, sizeof(DEVICE_MODE_INFO));
		memcpy(stDeviceMode.Id, deviceId, DEVICE_ID_LEN);
		memcpy(stDeviceMode.Id + DEVICE_ID_LEN, szCurrentMode, DEVICE_MODE_LEN);
		if(SYS_STATUS_SUCCESS != GetDeviceModeNode(stDeviceMode))
		{
			DBG(("GetDeviceModeNode [%s] not exist szCurrentMode[%s]\n", stDeviceMode.Id, szCurrentMode));
			return ret;
		}
		//{设备编号+模式编号+值编号，模式名称，值名称 }
		sprintf(stPollNode.ParaValues, "{%s%s,%s,%s}", stDeviceMode.Id, stDeviceMode.curActionId, stDeviceMode.szModeName, stDeviceMode.szActionName);
		
		char tempValue[1024] = {0};
		HashedDeviceMode::iterator it;
		DBG(("GetDeviceModesInfo  curMode[%s]\n", stPollNode.ParaValues));
		CTblLockMode.lock();
		for (it = m_HashTableMode->begin(); it != m_HashTableMode->end(); it++)
		{
			PDEVICE_MODE_INFO device_node = (PDEVICE_MODE_INFO)(&it->second);
			if(0 == strncmp(device_node->Id, stDeviceMode.Id, DEVICE_MODEID_LEN))
				continue;

			if (0 == strncmp(device_node->Id, deviceId, DEVICE_ID_LEN))//
			{
				DBG(("GetDeviceParasInfo deviceId[%s] \n", device_node->Id));

				sprintf(tempValue, "{%s%s,%s,%s}",
					device_node->Id, device_node->curActionId, device_node->szModeName,device_node->szActionName);

				if (strlen(tempValue) + strlen(stPollNode.ParaValues) > MAXMSGSIZE - 28)
				{
					if (0 < strlen(stPollNode.ParaValues))
					{
						char outBuf[MAXMSGSIZE] = {0};
						memcpy(outBuf, (char*)msg + MSGHDRLEN, 28);
						memcpy(outBuf+20, "0001", 4);
						strcat(outBuf, stPollNode.ParaValues);
						Net_App_Resp_Proc(msg, outBuf, strlen(outBuf), seq++);
					}
					memset(stPollNode.ParaValues, 0, sizeof(stPollNode.ParaValues));
					if (strlen(tempValue) <= MAXMSGSIZE - 28)
					{
						strcat(stPollNode.ParaValues, tempValue);
					}
				}
				else
					strcat(stPollNode.ParaValues, tempValue);
			}
		}
		CTblLockMode.unlock();
	}
//PrintPollHash();
	DBG(("GetDeviceInfoNode: id End[%s]\n", deviceId));
	return ret;
}


int CDevInfoContainer::GetDeviceInfoNode(char* deviceId, DEVICE_DETAIL& stPollNode)
{
	int ret = SYS_STATUS_FAILED;
	bool isDeviceExist = false;
	bool isDisarm = false;//撤防
	bool isOffline = false;//离线
	bool isException = false;//异常
	bool isMainShow = false;
	int aliasStatus = 3;

	char szShowStatus[56] = {0};//

	memset((unsigned char*)&stPollNode, 0, sizeof(DEVICE_DETAIL));

	//获取设备信息
	
	HashedDeviceDetail::iterator itDetail;
	DEVICE_DETAIL_INFO stDeviceDetail;
	memset((unsigned char*)&stDeviceDetail, 0, sizeof(DEVICE_DETAIL_INFO));
	memcpy(stDeviceDetail.Id, deviceId, DEVICE_ID_LEN);
	CTblLockDevice.lock();
	itDetail = m_HashTableDevice->find(stDeviceDetail.Id);	
	if (itDetail != m_HashTableDevice->end())
	{
		PDEVICE_DETAIL_INFO pNode = (PDEVICE_DETAIL_INFO)(&itDetail->second);
		memcpy((unsigned char*)&stDeviceDetail, (unsigned char*)pNode, sizeof(DEVICE_DETAIL_INFO));
		strcpy(stPollNode.szCName, stDeviceDetail.szCName);
		isDeviceExist = true;
	}
	CTblLockDevice.unlock();

	if (!isDeviceExist)
	{
		return ret;
	}

	ret = SYS_STATUS_SUCCESS;
	if (DEVICE_NOT_USE == stDeviceDetail.cStatus)
	{
		isDisarm = true;
	}

	if (DEVICE_OFFLINE == stDeviceDetail.cStatus)
	{
		isOffline = true;
	}

	if (!isDisarm && !isOffline)//撤防或离线时不需要显示属性值
	{
		HashedDeviceRoll::iterator it;
		char szShow[512] = {0};//
		CTblLockRoll.lock();
		for (it = m_HashTableRoll->begin(); it != m_HashTableRoll->end(); it++)
		{
			memset(szShow, 0, sizeof(szShow));
			PDEVICE_INFO_POLL device_node = (PDEVICE_INFO_POLL)(&it->second);
			if (0 == strncmp(device_node->Id, deviceId, DEVICE_ID_LEN) && '0' != device_node->cBeOn)//属性同属于一个设备，并且在采集
			{
				DBG(("GetDeviceInfoNode: id[%s]DataStatus[%d]\n", deviceId, device_node->stLastValueInfo.DataStatus));
				int attrStatus = DEVICE_USE;
				if (0 == strcmp(device_node->stLastValueInfo.Value, "NULL")
					|| 1 == device_node->stLastValueInfo.DataStatus)//有离线或异常
				{
					attrStatus = DEVICE_EXCEPTION;
					isException = true;
				}
				//获取
				char tempValue[1024] = {0};
				GetShowString(device_node->stLastValueInfo.Value, device_node->szS_Define, szShow);//不是状态属性
				
				if (0 == strcmp(device_node->stLastValueInfo.Value, "NULL") || 0 == strlen(device_node->stLastValueInfo.Value))
				{
					sprintf(tempValue, "{%s,%s,%s,%s,%d,%c}",device_node->Id,device_node->szAttrName, "无数据!","NULL",device_node->stLastValueInfo.DataStatus,device_node->cStaShow);
				}
				else
				{
					if('1' == device_node->cStaShow)
					{
						isMainShow = true;
						memcpy(szShowStatus, szShow, sizeof(szShowStatus));
					}
					sprintf(tempValue, "{%s,%s,%s%s,%s,%d,%c}",
										device_node->Id, 
										device_node->szAttrName, 
										szShow, m_ParaInfo[atoi(device_node->stLastValueInfo.Type)].szUnit, 
										device_node->stLastValueInfo.Value,
										device_node->stLastValueInfo.DataStatus,
										device_node->cStaShow);
				}

				if (strlen(tempValue) + strlen(stPollNode.ParaValues) < 1024)
				{
					strcat(stPollNode.ParaValues, tempValue);
				}
				if('1' == device_node->cStaShow)//是显示属性
				{
					if (0 != strcmp(device_node->stLastValueInfo.Value, "NULL")
						&& SYS_STATUS_SUCCESS == GetShowString(device_node->stLastValueInfo.Value, device_node->szS_Define, szShow))
					{
						isMainShow = true;
						strcat(szShow, m_ParaInfo[atoi(device_node->stLastValueInfo.Type)].szUnit);
						DBG(("GetDeviceStatusInfo显示属性 [%s][%s]\n", device_node->stLastValueInfo.Value, device_node->szAttrValueLimit));
						if (DEVICE_USE == attrStatus)
						{
							char* savePtr = NULL;
							char* split_result = NULL;
							char szAlias[6] = {0};
							char attrValueLimit[65] = {0};
							strcpy(attrValueLimit, device_node->szAttrValueLimit);
							sprintf(szAlias, "%d", attrStatus);
							split_result = strtok_r(attrValueLimit, ";", &savePtr);
							while(split_result != NULL)
							{
								char* pTemp = strstr(split_result, ",");
								if (NULL == pTemp)
								{
									break;
								}
								char szTemp[32] = {0};
								strncpy(szTemp, split_result, pTemp - split_result);
								if (atoi(szTemp) == atoi(device_node->stLastValueInfo.Value))
								{
									strcpy(szAlias,pTemp + 1);
									break;
								}
								split_result = strtok_r(NULL, ";", &savePtr);
							}
							aliasStatus = atoi(szAlias);
						}			
						else if (DEVICE_EXCEPTION == attrStatus)
						{
							aliasStatus = attrStatus;
						}
					}
				}
				else	//不是显示属性
				{
					aliasStatus = attrStatus;
				}
			}
		}
		CTblLockRoll.unlock();
	}
	strncpy(stPollNode.Id, deviceId, DEVICE_ID_LEN);

	if (isDisarm)//撤防
	{
		char tempShow[57] = {0};
		sprintf(tempShow, "%-56s", "撤防");
		memcpy(stPollNode.Show, tempShow, 56);
		stPollNode.Status = DEVICE_NOT_USE;
	}
	else if (isOffline)//离线
	{
		char tempShow[57] = {0};
		sprintf(tempShow, "%-56s", "离线");
		memcpy(stPollNode.Show, tempShow, 56);

		stPollNode.Status = DEVICE_OFFLINE;
	}else if (isException)//异常
	{
		stPollNode.Status = DEVICE_EXCEPTION;
		if (isMainShow)
		{
			char tempShow[57] = {0};
			sprintf(tempShow, "%-56s", szShowStatus);
			memcpy(stPollNode.Show, tempShow, 56);
			stPollNode.Status = aliasStatus;
		}
		else
		{
			char tempShow[57] = {0};
			sprintf(tempShow, "%-56s", "异常");
			memcpy(stPollNode.Show, tempShow, 56);
		}
	}
	else
	{
		stPollNode.Status = DEVICE_USE;
		if (isMainShow)
		{
			char tempShow[57] = {0};
			sprintf(tempShow, "%-56s", szShowStatus);
			memcpy(stPollNode.Show, tempShow, 56);
			stPollNode.Status = aliasStatus;
		}
		else
		{
			char tempShow[57] = {0};
			sprintf(tempShow, "%-56s", "正常");
			memcpy(stPollNode.Show, tempShow, 56);
		}
	}
	return ret;
}
int CDevInfoContainer::GetDeviceRealInfo(char* deviceId, DEVICE_DETAIL& stPollNode)
{
	bool bHaveMainShow = false;
	HashedDeviceRoll::iterator it;
	DEVICE_INFO_POLL stDeviceInfoPoll;
	memset((unsigned char*)&stDeviceInfoPoll, 0, sizeof(DEVICE_INFO_POLL));

	memset((unsigned char*)&stPollNode, 0, sizeof(DEVICE_DETAIL));
	CTblLockRoll.lock();
	for (it = m_HashTableRoll->begin(); it != m_HashTableRoll->end(); it++)
	{
		PDEVICE_INFO_POLL device_node = (PDEVICE_INFO_POLL)(&it->second);
		if (0 == strncmp(device_node->Id, deviceId, DEVICE_ID_LEN) && '0' != device_node->cBeOn)//属性同属于一个设备，并且在采集
		{
			if('1' == device_node->cStaShow)//是显示属性
			{
				memcpy((unsigned char*)&stDeviceInfoPoll, (unsigned char*)device_node, sizeof(DEVICE_INFO_POLL));
				bHaveMainShow = true;
				break;
			}
		}
	}
	CTblLockRoll.unlock();
	if (bHaveMainShow)//实时同步查询
	{
		if (DEVICE_ATTR_ID_LEN == strlen(stDeviceInfoPoll.V_Id))
		{
			CDevInfoContainer::DisPatchQueryAction(stDeviceInfoPoll.V_Id);
		}
		CDevInfoContainer::DisPatchQueryAction(stDeviceInfoPoll.Id);
	}
	return GetDeviceInfoNode(deviceId, stPollNode);
}

int CDevInfoContainer::GetShowString(char* szValue, char* pDefine, char* outBuf)
{
	int ret = SYS_STATUS_FAILED;
	char* savePtr = NULL;
	char* split_result = NULL;

	if (3 > strlen(pDefine))
	{
		strcpy(outBuf, trim(szValue, strlen(szValue)));
		return ret;
	}

	char szDefine[128] = {0};
	strcpy(szDefine, pDefine);
	DBG(("-----szValue[%s]---------szDefine[%s] ---------------\n", szValue, szDefine));
	bool error = false;
	split_result = strtok_r(szDefine, ";", &savePtr);
	while(split_result != NULL)
	{
		if (strlen(split_result)<3)
		{
			error = true;
			break;
		}
		char* pTemp = strstr(split_result, ",");
		if (NULL == pTemp)
		{
			error = true;
			break;
		}
		char szTemp[32] = {0};
		strncpy(szTemp, split_result, pTemp - split_result);
		if (atoi(szTemp) == atoi(szValue))
		{
			strcpy(outBuf,pTemp + 1);
			ret = SYS_STATUS_SUCCESS;
			DBG(("-----[%s]---------[%s] --------------[%s]----\n", outBuf, szTemp, szValue));
			break;
		}
		split_result = strtok_r(NULL, ";", &savePtr);
	}
	if (error)
	{
		strcpy(outBuf, szValue);
	}
	return ret;
}
int CDevInfoContainer::GetAttrNode(char* deviceAttrId, DEVICE_INFO_POLL& stPollNode)
{
	int ret = SYS_STATUS_FAILED;
	DEVICE_INFO_POLL tempPollNode;
	memset((unsigned char*)&tempPollNode, 0, sizeof(DEVICE_INFO_POLL));
	strcpy(tempPollNode.Id, deviceAttrId);
	CTblLockRoll.lock();
	HashedDeviceRoll::iterator it = m_HashTableRoll->find(tempPollNode.Id);
	if (m_HashTableRoll->end() != it)
	{
		//执行动作
		PDEVICE_INFO_POLL pNode = (PDEVICE_INFO_POLL)(&it->second);
		memcpy((unsigned char*)&stPollNode, (unsigned char*)pNode, sizeof(DEVICE_INFO_POLL));
		ret = SYS_STATUS_SUCCESS;
	}
	CTblLockRoll.unlock();
	return ret;
}

int CDevInfoContainer::GetNextAttrNode(char* startAttr, char* deviceAttrId, DEVICE_INFO_POLL& stPollNode)
{
	int ret = SYS_STATUS_FAILED;
	char strNextAttrId[DEVICE_ATTR_ID_LEN_TEMP] = {0}; 
	strncpy(strNextAttrId, deviceAttrId, DEVICE_ATTR_ID_LEN);
	DEVICE_INFO_POLL tempPollNode;
	CTblLockRoll.lock();
	while(DEVICE_ATTR_ID_LEN == strlen(strNextAttrId) && 0 != strcmp(strNextAttrId, stPollNode.Id))
	{
		if(0 == strncmp(startAttr, strNextAttrId, DEVICE_ATTR_ID_LEN))
			break;
		memset((unsigned char*)&tempPollNode, 0, sizeof(DEVICE_INFO_POLL));
		strcpy(tempPollNode.Id, strNextAttrId);
		HashedDeviceRoll::iterator it = m_HashTableRoll->find(tempPollNode.Id);
		if (m_HashTableRoll->end() != it)
		{
			PDEVICE_INFO_POLL pNode = (PDEVICE_INFO_POLL)(&it->second);
			if (pNode->cBeOn == '0')//未启用
			{
				strncpy(strNextAttrId, pNode->NextId, DEVICE_ATTR_ID_LEN);
				continue;
			}
			memcpy((unsigned char*)&stPollNode, (unsigned char*)pNode, sizeof(DEVICE_INFO_POLL));
			ret = SYS_STATUS_SUCCESS;
			break;
		}
		else
			break;
	}
	CTblLockRoll.unlock();
	return ret;
}

int CDevInfoContainer::GetAttrValue(char* deviceAttrId, LAST_VALUE_INFO& stLastValue)
{
	int ret = SYS_STATUS_FAILED;
	DEVICE_INFO_POLL stPollNode;
	memset((unsigned char*)&stPollNode, 0, sizeof(DEVICE_INFO_POLL));
	strcpy(stPollNode.Id, deviceAttrId);
	CTblLockRoll.lock();
	HashedDeviceRoll::iterator it = m_HashTableRoll->find(stPollNode.Id);
	if (m_HashTableRoll->end() != it)
	{
		//执行动作
		PDEVICE_INFO_POLL pNode = (PDEVICE_INFO_POLL)(&it->second);
		memcpy((unsigned char*)&stLastValue, (unsigned char*)&pNode->stLastValueInfo, sizeof(LAST_VALUE_INFO));
		ret = SYS_STATUS_SUCCESS;
	}
	CTblLockRoll.unlock();
	return ret;
}

bool CDevInfoContainer::IsDeviceOnline(char* deviceId)
{
	bool ret = false;
	HashedDeviceRoll::iterator it;
	PDEVICE_INFO_POLL pNode = NULL;
	DBG(("判断设备是否离线\n"));
	CTblLockRoll.lock();
	for (it = m_HashTableRoll->begin(); it != m_HashTableRoll->end(); it++)
	{
		pNode = (PDEVICE_INFO_POLL)(&it->second);
//		DBG(("deviceId[%s] NodeId[%s] OffLineCnt[%d] v_id[%s]\n",deviceId, pNode->Id, pNode->offLineCnt, pNode->V_Id));
		if (0 != strncmp(pNode->Id, deviceId, DEVICE_ATTR_ID_LEN)
			&&0 == strncmp(pNode->Id, deviceId, DEVICE_ID_LEN)
			&& 0 < pNode->offLineCnt 
			&& '1' == pNode->cBeOn		//属性需要采集
			&& 0 == strlen(pNode->V_Id))//虚属性不参与判断
		{
			DBG(("设备在线 [%s][%s][%d][%s]\n", pNode->Id, deviceId, pNode->offLineCnt, pNode->V_Id));
			ret = true;
			break;
		}
	}
	CTblLockRoll.unlock();
	return ret;
}

int CDevInfoContainer::AddDeviceRoll(DEVICE_INFO_POLL node)
{
	int ret = -1;
	return ret;
}
int CDevInfoContainer::UpdateDeviceRoll(DEVICE_INFO_POLL node)
{
	int ret = SYS_STATUS_FAILED;
	HashedDeviceRoll::iterator it;
	DEVICE_INFO_POLL oldNode;
	memset((unsigned char*)&oldNode, 0, sizeof(DEVICE_INFO_POLL));
	CTblLockRoll.lock();
	it = m_HashTableRoll->find(node.Id);
	if (it != m_HashTableRoll->end())
	{
		PDEVICE_INFO_POLL pNode = (PDEVICE_INFO_POLL)(&it->second);
		memcpy((unsigned char*)&oldNode, (unsigned char*)pNode, sizeof(DEVICE_INFO_POLL));
		
		if (atoi(oldNode.Upper_Channel) == atoi(node.Upper_Channel))
		{
			//if (oldNode.offLineCnt != node.offLineCnt)
			//更新最新查询的时间
			oldNode.offLineCnt = node.offLineCnt;
			oldNode.retryCount = 0;
			memcpy((unsigned char*)(&oldNode.stLastValueInfo), (unsigned char*)(&node.stLastValueInfo), sizeof(LAST_VALUE_INFO));
			//DBG(("id[%s] old offLine Cnt[%d] new offLine Cnt[%d] status[%d] oldTime[%d], newtime[%d]\n", 
			//	node.Id, oldNode.offLineCnt, node.offLineCnt, oldNode.stLastValueInfo.DataStatus, oldNode.stLastValueInfo.LastTime, node.stLastValueInfo.LastTime));
			m_HashTableRoll->erase(it);
			ret = true;
			m_HashTableRoll->insert(HashedDeviceRoll::value_type(oldNode.Id,oldNode));
			ret = SYS_STATUS_SUCCESS;
		}
	}
	CTblLockRoll.unlock();
	return ret;
}

int CDevInfoContainer::UpdateDeviceRollRetryCount(DEVICE_INFO_POLL node)
{	
	int ret = SYS_STATUS_FAILED;
	HashedDeviceRoll::iterator it;
	DEVICE_INFO_POLL oldNode;
	memset((unsigned char*)&oldNode, 0, sizeof(DEVICE_INFO_POLL));
	CTblLockRoll.lock();
	it = m_HashTableRoll->find(node.Id);
	if (it != m_HashTableRoll->end())
	{
		PDEVICE_INFO_POLL pNode = (PDEVICE_INFO_POLL)(&it->second);
		memcpy((unsigned char*)&oldNode, (unsigned char*)pNode, sizeof(DEVICE_INFO_POLL));
		
		oldNode.retryCount = node.retryCount;
		m_HashTableRoll->erase(it);
		ret = true;
		m_HashTableRoll->insert(HashedDeviceRoll::value_type(oldNode.Id,oldNode));
		ret = SYS_STATUS_SUCCESS;
	}
	CTblLockRoll.unlock();
	return ret;
}
int CDevInfoContainer::UpdateDeviceRollLastValue(DEVICE_INFO_POLL node, char* lastValue)
{
	int ret = SYS_STATUS_FAILED;
	HashedDeviceRoll::iterator it;
	DEVICE_INFO_POLL oldNode;
	memset((unsigned char*)&oldNode, 0, sizeof(DEVICE_INFO_POLL));
	CTblLockRoll.lock();
	it = m_HashTableRoll->find(node.Id);
	if (it != m_HashTableRoll->end())
	{
		PDEVICE_INFO_POLL pNode = (PDEVICE_INFO_POLL)(&it->second);
		memcpy((unsigned char*)&oldNode, (unsigned char*)pNode, sizeof(DEVICE_INFO_POLL));
		memset(oldNode.stLastValueInfo.Value, 0, sizeof(oldNode.stLastValueInfo.Value));
		strcpy(oldNode.stLastValueInfo.Value, lastValue);
		m_HashTableRoll->erase(it);
		m_HashTableRoll->insert(HashedDeviceRoll::value_type(oldNode.Id,oldNode));
		ret = SYS_STATUS_SUCCESS;
	}
	else
	{
		DBG(("UpdateDeviceRollLastValue find failed deviceId[%s]\n", node.Id));
	}
	CTblLockRoll.unlock();
	return ret;
}
int CDevInfoContainer::ADCValueNoticeCallBack(void* data, int iChannel, unsigned char* value, int len)
{
	CDevInfoContainer* _this = (CDevInfoContainer*)data;
	return _this->UpdateDeviceRollNodeByChannel(iChannel, value, len, true);
}
int CDevInfoContainer::UpdateDeviceRollNodeByChannel(int iChannel, unsigned char* value, int len, bool isSend)
{
	int ret = SYS_STATUS_FAILED;
	HashedDeviceRoll::iterator it;
	PDEVICE_INFO_POLL device_node;

	time_t tm_NowTime;
	time(&tm_NowTime);  

	DEVICE_INFO_POLL deviceNode;
	if(SYS_STATUS_SUCCESS != GetDeviceRollByChannelId(iChannel, deviceNode))
		return ret;
	//更新设备表中的当前模式
	DEVICE_DETAIL_INFO stDeviceDetail;
	memset((unsigned char*)&stDeviceDetail, 0, sizeof(DEVICE_DETAIL_INFO));
	memcpy(stDeviceDetail.Id, deviceNode.Id, DEVICE_ID_LEN);
	if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetDeviceDetailNode(stDeviceDetail))
		return ret;			
	if(DEVICE_NOT_USE == stDeviceDetail.cStatus)//设备已撤防
		return ret;

	device_node = (PDEVICE_INFO_POLL)(&deviceNode);

	if (iChannel == atoi(device_node->Upper_Channel) && device_node->cBeOn != '0')
	{
		QUERY_INFO_CPM stQueryValue;
		memset((unsigned char*)&stQueryValue, 0, sizeof(QUERY_INFO_CPM));
		Time2Chars(&tm_NowTime, stQueryValue.Time);
		stQueryValue.Cmd = 0x00;//轮巡到的数据
		memcpy(stQueryValue.DevId, device_node->Id,  DEVICE_ATTR_ID_LEN);

		QUERY_INFO_CPM_EX stQueryValueEx;
		memset((unsigned char*)&stQueryValueEx, 0, sizeof(QUERY_INFO_CPM_EX));
		Time2Chars(&tm_NowTime, stQueryValueEx.Time);
		stQueryValueEx.Cmd = 0x00;//轮巡到的数据
		memcpy(stQueryValueEx.DevId, device_node->Id,  DEVICE_ATTR_ID_LEN);

		int iDecRslt = LabelIF_DecodeByFormat(device_node, value, len, &stQueryValueEx);
		if (SYS_STATUS_DATA_DECODE_NORMAL == iDecRslt)
		{	
			//校验并解析成功,置最新数据
			memcpy((unsigned char*)&stQueryValue, (unsigned char*)&stQueryValueEx, sizeof(QUERY_INFO_CPM));
			memcpy(stQueryValue.Value, stQueryValueEx.Value, sizeof(stQueryValue.Value));
			stQueryValue.DataStatus = stQueryValueEx.DataStatus;
			printf("DeviceInfoContainer::UpdateDeviceRollNodeByChannel - stQueryValue.DataStatus[%d]\n", stQueryValue.DataStatus);
			//校验并解析成功,置最新数据
			struct timeval tmval_NowTime;
			gettimeofday(&tmval_NowTime, NULL);
			memcpy(device_node->stLastValueInfo.Type, stQueryValue.Type, 4);
			device_node->stLastValueInfo.VType = stQueryValue.VType;
			memcpy(device_node->stLastValueInfo.Value, stQueryValue.Value, sizeof(device_node->stLastValueInfo.Value));

			stQueryValue.DataStatus = LabelIF_StandarCheck(stQueryValue.VType, stQueryValue.Value, device_node->szStandard)?0:1;
			device_node->stLastValueInfo.DataStatus = stQueryValue.DataStatus;
			DBG(("CDevInfoContainer::UpdateDeviceRollNodeByChannel: id[%s] value[%s] time[%s] Status[%d]\n", stQueryValue.DevId, stQueryValue.Value, stQueryValue.Time, stQueryValue.DataStatus));
			UpdateDeviceRoll(deviceNode);
			if (isSend)
			{
				memcpy((unsigned char*)(&(device_node->stLastValueInfo.LastTime)), (unsigned char*)(&tmval_NowTime), sizeof(struct timeval));
				if(!SendMsg_Ex(g_ulHandleAnalyser, MUCB_DEV_ANALYSE, (char*)&stQueryValue, sizeof(QUERY_INFO_CPM)))
				{
					DBG(("DeviceIfBase DecodeAndSendToAnalyser 2 SendMsg_Ex failed  handle[%d]\n", g_ulHandleAnalyser));
				}
			}
		}
		else
		{
			device_node->offLineCnt --;
			if (SYS_STATUS_DATA_DECODE_UNNOMAL_AGENT == iDecRslt)
			{
				struct timeval tmval_NowTime;
				gettimeofday(&tmval_NowTime, NULL);
				memcpy((unsigned char*)(&(device_node->stLastValueInfo.LastTime)), (unsigned char*)(&tmval_NowTime), sizeof(struct timeval));
				if(!SendMsg_Ex(g_ulHandleAnalyser, MUCB_DEV_ANALYSE, (char*)&stQueryValue, sizeof(QUERY_INFO_CPM)))
				{
					DBG(("DeviceIfBase DecodeAndSendToAnalyser 2 SendMsg_Ex failed  handle[%d]\n", g_ulHandleAnalyser));
				}
			}
		}
	}
	ret = SYS_STATUS_SUCCESS;
	return ret;
}

int CDevInfoContainer::UpdateDeviceRollNodeByChannelDevType(int iChannel, char* devType, char* attrId,  unsigned char* value, int len, bool isSend)
{
	int ret = SYS_STATUS_FAILED;
	HashedDeviceRoll::iterator it;
	PDEVICE_INFO_POLL device_node;

	time_t tm_NowTime;
	time(&tm_NowTime);  

	DEVICE_INFO_POLL deviceNode;
//	if(SYS_STATUS_SUCCESS != GetDeviceRollByChannelDevTypeId(iChannel, devType, attrId, deviceNode))
	if(SYS_STATUS_SUCCESS != GetDeviceRollBySpecialDevTypeId(devType, attrId, deviceNode))		
		return ret;
	//更新设备表中的当前模式
	DEVICE_DETAIL_INFO stDeviceDetail;
	memset((unsigned char*)&stDeviceDetail, 0, sizeof(DEVICE_DETAIL_INFO));
	memcpy(stDeviceDetail.Id, deviceNode.Id, DEVICE_ID_LEN);
	if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetDeviceDetailNode(stDeviceDetail))
		return ret;			
	if(DEVICE_NOT_USE == stDeviceDetail.cStatus)//设备已撤防
		return ret;

	device_node = (PDEVICE_INFO_POLL)(&deviceNode);

	//if (iChannel == atoi(device_node->Upper_Channel) && device_node->cBeOn != '0')
	if (device_node->cBeOn != '0')
	{
		QUERY_INFO_CPM stQueryValue;
		memset((unsigned char*)&stQueryValue, 0, sizeof(QUERY_INFO_CPM));
		Time2Chars(&tm_NowTime, stQueryValue.Time);
		stQueryValue.Cmd = 0x00;//轮巡到的数据
		memcpy(stQueryValue.DevId, device_node->Id,  DEVICE_ATTR_ID_LEN);
		
		QUERY_INFO_CPM_EX stQueryValueEx;
		memset((unsigned char*)&stQueryValueEx, 0, sizeof(QUERY_INFO_CPM_EX));
		Time2Chars(&tm_NowTime, stQueryValueEx.Time);
		stQueryValueEx.Cmd = 0x00;//轮巡到的数据
		memcpy(stQueryValueEx.DevId, device_node->Id,  DEVICE_ATTR_ID_LEN);

		int iDecRslt = LabelIF_DecodeByFormat(device_node, value, len, &stQueryValueEx);
		if (SYS_STATUS_DATA_DECODE_NORMAL == iDecRslt)
		{	
			memcpy((unsigned char*)&stQueryValue, (unsigned char*)&stQueryValueEx, sizeof(QUERY_INFO_CPM));
			memcpy(stQueryValue.Value, stQueryValueEx.Value, sizeof(stQueryValue.Value));
			stQueryValue.DataStatus = stQueryValueEx.DataStatus;

			//校验并解析成功,置最新数据
			struct timeval tmval_NowTime;
			gettimeofday(&tmval_NowTime, NULL);
			memcpy(device_node->stLastValueInfo.Type, stQueryValue.Type, 4);
			device_node->stLastValueInfo.VType = stQueryValue.VType;
			memcpy(device_node->stLastValueInfo.Value, stQueryValue.Value, sizeof(device_node->stLastValueInfo.Value));

			stQueryValue.DataStatus = LabelIF_StandarCheck(stQueryValue.VType, stQueryValue.Value, device_node->szStandard)?0:1;
			device_node->stLastValueInfo.DataStatus = stQueryValue.DataStatus;
			printf("CDevInfoContainer::UpdateDeviceRollNodeByChannel: id[%s] value[%s] time[%s] Status[%d]\n", stQueryValue.DevId, stQueryValue.Value, stQueryValue.Time, stQueryValue.DataStatus);
			UpdateDeviceRoll(deviceNode);
			if (isSend)
			{
				memcpy((unsigned char*)(&(device_node->stLastValueInfo.LastTime)), (unsigned char*)(&tmval_NowTime), sizeof(struct timeval));
				if(!SendMsg_Ex(g_ulHandleAnalyser, MUCB_DEV_ANALYSE, (char*)&stQueryValue, sizeof(QUERY_INFO_CPM)))
				{
					DBG(("DeviceIfBase UpdateDeviceRollNodeByChannelDevType 2 SendMsg_Ex failed  handle[%d]\n", g_ulHandleAnalyser));
				}
			}
		}
		else
		{
			device_node->offLineCnt --;
			if (SYS_STATUS_DATA_DECODE_UNNOMAL_AGENT == iDecRslt)
			{
				struct timeval tmval_NowTime;
				gettimeofday(&tmval_NowTime, NULL);
				memcpy((unsigned char*)(&(device_node->stLastValueInfo.LastTime)), (unsigned char*)(&tmval_NowTime), sizeof(struct timeval));
				if(!SendMsg_Ex(g_ulHandleAnalyser, MUCB_DEV_ANALYSE, (char*)&stQueryValue, sizeof(QUERY_INFO_CPM)))
				{
					DBG(("DeviceIfBase DecodeAndSendToAnalyser 2 SendMsg_Ex failed  handle[%d]\n", g_ulHandleAnalyser));
				}
			}
		}
	}

	ret = SYS_STATUS_SUCCESS;
	return ret;
}
int CDevInfoContainer::UpdateDeviceAlarmValue(DEVICE_INFO_POLL node)
{
	bool ret = false;
	HashedDeviceRoll::iterator it;
	DEVICE_INFO_POLL oldNode;
	memset((unsigned char*)&oldNode, 0, sizeof(DEVICE_INFO_POLL));
	CTblLockRoll.lock();
	it = m_HashTableRoll->find(node.Id);
	if (it != m_HashTableRoll->end())
	{
		PDEVICE_INFO_POLL pNode = (PDEVICE_INFO_POLL)(&it->second);
		memcpy((unsigned char*)&oldNode, (unsigned char*)pNode, sizeof(DEVICE_INFO_POLL));

		if (atoi(oldNode.Upper_Channel) == atoi(node.Upper_Channel))
		{
			memcpy(oldNode.stLastValueInfo.szLevel, node.stLastValueInfo.szLevel, 4);
			memcpy(oldNode.stLastValueInfo.szDescribe, node.stLastValueInfo.szDescribe, 128);
			m_HashTableRoll->erase(it);
			ret = true;
			m_HashTableRoll->insert(HashedDeviceRoll::value_type(oldNode.Id,oldNode));
		}
	}
	CTblLockRoll.unlock();
	return ret;
}

int CDevInfoContainer::DeleteDeviceRoll(DEVICE_INFO_POLL node)
{
	int ret = -1;
	return ret;
}


/******************************************************************************/
/* Function Name  : QueryHashDeleteByDevice                                                */
/* Description    : 删除轮巡设备表节点                                        */
/* Input          : NET_DEVICE_INFO_UPDATE new_node      设备节点                       */
/* Output         : none                                                      */
/* Return         : true   节点更新                                           */
/*                  false  节点增加                                           */
/******************************************************************************/
int CDevInfoContainer::QueryHashDeleteByDevice(NET_DEVICE_INFO_UPDATE new_node)
{
	int ret = SYS_STATUS_FAILED;
	HashedDeviceRoll::iterator it;
	PDEVICE_INFO_POLL device_node;
	DEVICE_INFO_POLL startNode[256];
	int startNodeCount = 0;

	CTblLockRoll.lock();

	for (it = m_HashTableRoll->begin(); it != m_HashTableRoll->end(); it++)
	{
		device_node = (PDEVICE_INFO_POLL)(&it->second);

		if (0 == strncmp(device_node->Id, new_node.Id, DEVICE_ID_LEN))
		{
			memcpy((unsigned char*)&startNode[startNodeCount++], (unsigned char*)device_node, sizeof(DEVICE_INFO_POLL));
		}
	}
	for(int i=0; i<startNodeCount; i++)
	{
		doDeleteQueryNode(startNode[i]);
	}
	CTblLockRoll.unlock();
	ret = SYS_STATUS_SUCCESS;
	return ret;
}

/******************************************************************************/
/* Function Name  : QueryHashDeleteByDeviceAttr                               */
/* Description    : 删除轮巡设备表节点                                        */
/* Input          : device_attr      采集属性编号                       */
/* Output         : none                                                      */
/* Return         : true   节点更新                                           */
/*                  false  节点增加                                           */
/******************************************************************************/
int CDevInfoContainer::QueryHashDeleteByDeviceAttr(char* device_attr)
{
	DEVICE_INFO_POLL hash_node;
	memset((unsigned char*)&hash_node, 0, sizeof(DEVICE_INFO_POLL));
	strncpy(hash_node.Id, device_attr, DEVICE_ATTR_ID_LEN);
	DeleteQueryNode(hash_node);
	return true;
}
/******************************************************************************/
/* Function Name  : QueryHashUpdateByDevice                                                */
/* Description    : 更新轮巡设备表节点                                        */
/* Input          : NET_DEVICE_INFO_UPDATE new_node      设备节点                       */
/* Output         : none                                                      */
/* Return         : true   节点更新                                           */
/*                  false  节点增加                                           */
/******************************************************************************/
int CDevInfoContainer::QueryHashUpdateByDevice(NET_DEVICE_INFO_UPDATE new_node)
{
	int ret = SYS_STATUS_FAILED;
	HashedDeviceRoll::iterator it;
	DBG(("QueryHashUpdateByDevice Start.\n"));

	DEVICE_INFO_POLL oldNodes[256];
	memset((unsigned char*)&oldNodes[0], 0, 256*sizeof(DEVICE_INFO_POLL));
	int startNodeCount = 0;
	bool bChannelChanged = false;
	int iOldChannel = 0;
	int iNewChannel = 0;
	CTblLockRoll.lock();

	//查找该设备采集属性的信息
	for (it = m_HashTableRoll->begin(); it != m_HashTableRoll->end(); it++)
	{
		PDEVICE_INFO_POLL pOldNode = (PDEVICE_INFO_POLL)(&it->second);

		if (0 == strncmp(pOldNode->Id, new_node.Id, DEVICE_ID_LEN))
		{
			memcpy((unsigned char*)&oldNodes[startNodeCount], (unsigned char*)pOldNode, sizeof(DEVICE_INFO_POLL));
			strcpy(oldNodes[startNodeCount].Upper_Channel, new_node.Upper_Channel);
			strcpy(oldNodes[startNodeCount].Self_Id, new_node.Self_Id);
			LabelIF_GetDeviceQueryPrivatePara(new_node.PrivateAttr, &oldNodes[startNodeCount]);

			DBG(("oldNodes[%d] id[%s]\n", startNodeCount, oldNodes[startNodeCount].Id));
			if (atoi(pOldNode->Upper_Channel) != atoi(new_node.Upper_Channel))
			{
				iOldChannel = atoi(pOldNode->Upper_Channel);
				iNewChannel = atoi(new_node.Upper_Channel);
				bChannelChanged = true;
			}
			memset(pOldNode->Upper_Channel, 0, sizeof(pOldNode->Upper_Channel));
			memset(pOldNode->Self_Id, 0, sizeof(pOldNode->Self_Id));
			strcpy(pOldNode->Upper_Channel, new_node.Upper_Channel);
			strcpy(pOldNode->Self_Id, new_node.Self_Id);
			LabelIF_GetDeviceQueryPrivatePara(new_node.PrivateAttr, pOldNode);
			startNodeCount++;
		}
	}

	if (0 < startNodeCount)
	{
		if (iOldChannel != iNewChannel)//如果通道发生了变化
		{	//删除属性
			for(int i= 0; i<startNodeCount; i++)
			{
				if(SYS_STATUS_SUCCESS == (ret = doDeleteQueryNode(oldNodes[i])))
				{
					//再插入
					ret = doInsertQueryNode(oldNodes[i]);
				}
			}
		}
		
		//如果涉及到网络接口
		if (INTERFACE_NET_INPUT <= iOldChannel || INTERFACE_NET_INPUT <= iNewChannel)
		{
			if(iOldChannel != iNewChannel || 0 != strcmp(oldNodes[0].Self_Id, new_node.Self_Id) )
			{
				g_NetContainer.deleteNode(oldNodes[0].Self_Id);
				if (INTERFACE_NET_INPUT <= iNewChannel)
				{
					char deviceId[DEVICE_ID_LEN_TEMP] = {0};
					strncpy(deviceId, new_node.Id, DEVICE_ID_LEN);
					g_NetContainer.addNode(deviceId);
				}
			}
		}
	}

	CTblLockRoll.unlock();

	DBG(("------------------1-----------------\n"));
	//PrintPollHash();
	DBG(("QueryHashUpdateByDevice End.\n"));
	return ret;
}

/******************************************************************************/
/* Function Name  : QueryHashUpdateByDeviceAttr                               */
/* Description    : 更新轮巡设备表节点                                        */
/* Input          : NET_DEVICE_ATTR_INFO_UPDATE new_node      设备节点        */
/* Output         : none                                                      */
/* Return         : true   节点更新                                           */
/*                  false  节点增加                                           */
/******************************************************************************/
int CDevInfoContainer::QueryHashUpdateByDeviceAttr(NET_DEVICE_ATTR_INFO_UPDATE new_node)
{
	int ret = SYS_STATUS_FAILED;

	DEVICE_INFO_POLL tempNode;
	memset((unsigned char*)&tempNode, 0, sizeof(DEVICE_INFO_POLL));
	strncpy(tempNode.Id, new_node.Id, DEVICE_ID_LEN);
	strncpy(tempNode.Id + DEVICE_ID_LEN, new_node.AttrId, DEVICE_SN_LEN);

	CTblLockRoll.lock();
	HashedDeviceRoll::iterator it = m_HashTableRoll->find(tempNode.Id);
	if (m_HashTableRoll->end() != it)
	{
		PDEVICE_INFO_POLL pNode = (PDEVICE_INFO_POLL)(&it->second);
		memcpy((unsigned char*)&tempNode, (unsigned char*)pNode, sizeof(DEVICE_INFO_POLL));
		LabelIF_GetDefenceDate(new_node.SecureTime, &tempNode);

		LabelIF_GetDeviceAttrQueryPrivatePara(new_node.PrivateAttr, &tempNode);		//设备属性私有参数

		/*
		char temp[64] = {0};
		int getParaRet = define_sscanf(new_node.PrivateAttr, "interval=",temp);       //获取采样频率

		if (0 < getParaRet)
		{
			if( 0>= (tempNode.Frequence = atoi(temp)))
			{
				tempNode.Frequence = 1000*1000*60;
			}
		}
		*/
		memset(tempNode.V_Id, 0, sizeof(tempNode.V_Id));
		strncpy(tempNode.V_Id, new_node.szV_Id, DEVICE_ID_LEN);
		strncpy(tempNode.V_Id + DEVICE_ID_LEN, new_node.szV_AttrId, DEVICE_SN_LEN);
		tempNode.cStaShow = new_node.cStaShow;
		memcpy(tempNode.szStandard, new_node.StandardValue, 60);
		strncpy(tempNode.PreId, tempNode.Id, DEVICE_ATTR_ID_LEN);
		strncpy(tempNode.NextId, tempNode.Id, DEVICE_ATTR_ID_LEN);
		strncpy(tempNode.ChannelPreId, tempNode.Id, DEVICE_ATTR_ID_LEN);
		strncpy(tempNode.ChannelNextId, tempNode.Id, DEVICE_ATTR_ID_LEN);

		tempNode.cBeOn = new_node.cStatus;
		memset(tempNode.szAttrName, 0, sizeof(tempNode.szAttrName));
		strcpy(tempNode.szAttrName, new_node.szAttr_Name);
		memset(tempNode.szS_Define, 0, sizeof(tempNode.szS_Define));
		strcpy(tempNode.szS_Define, new_node.szS_define);
		memset(tempNode.szValueLimit, 0, sizeof(tempNode.szValueLimit));
		LabelIF_GetDeviceValueLimit(new_node.szValueLimit, &tempNode);			//设备私有参数

		if(SYS_STATUS_SUCCESS == (ret = doDeleteQueryNode(tempNode)))
		{
			ret = doInsertQueryNode(tempNode);
		}
	}

	CTblLockRoll.unlock();
	return ret;
}

bool CDevInfoContainer::PrintPollHash()
{
	HashedDeviceRoll::iterator it;
	PDEVICE_INFO_POLL pNewNode;
	DBG(("********************当前设备轮巡表*******************\n"));
	CTblLockRoll.lock();
	for (it = m_HashTableRoll->begin(); it != m_HashTableRoll->end(); it++)
	{
		pNewNode = (PDEVICE_INFO_POLL)(&it->second);
		DBG(("ID[%s] channelId[%s] attrNextId[%s] attrPreId[%s] channelNextId [%s] channelPreId[%s]", pNewNode->Id, pNewNode->Upper_Channel, pNewNode->NextId, pNewNode->PreId, pNewNode->ChannelNextId, pNewNode->ChannelPreId));
		DBG(("		最新数据[%s] 数据状态[%d]\n", pNewNode->stLastValueInfo.Value, pNewNode->stLastValueInfo.DataStatus));
	}
	CTblLockRoll.unlock();
	return true;
}

/******************************************************************************/
/* Function Name  : delNode                                                   */
/* Description    : 从设备表里删除节点                                        */
/* Input          : DEVICE_INFO hash_node      设备节点                       */
/* Output         : none                                                      */
/* Return         : true   节点删除成功                                       */
/*                  false  节点删除失败                                       */
/******************************************************************************/
bool CDevInfoContainer::delNode(DEVICE_INFO_POLL hash_node)
{
	HashedDeviceRoll::iterator it;
	CTblLockRoll.lock();
	it = m_HashTableRoll->find(hash_node.Id);	
	if (it != m_HashTableRoll->end())
	{
		m_HashTableRoll->erase(it);
	}
	CTblLockRoll.unlock();
	return true;
}

/******************************************************************************/
/* Function Name  : GetHashData                                               */
/* Description    : 从哈希表中寻找节点并取出内容                              */
/* Input          : hash_node     哈希节点                                    */
/* Output         : none                                                      */
/* Return         : true     查找成功                                         */
/*                  false    查找失败                                         */
/******************************************************************************/
bool CDevInfoContainer::GetHashData(DEVICE_INFO_POLL& hash_node)
{
	bool ret = false;
	HashedDeviceRoll::iterator it;
	CTblLockRoll.lock();

	it = m_HashTableRoll->find(hash_node.Id);
	if (it != m_HashTableRoll->end())
	{
		memcpy((BYTE*)&hash_node, (BYTE*)&it->second.Id[0], sizeof(DEVICE_INFO_POLL));
		ret = true;
	}

	CTblLockRoll.unlock();
	return ret;
}


/******************************************************************************/
/* Function Name  : HashDeviceDefenceSet                                                */
/* Description    : 设备布防撤防                                        */
/* Input          : char* device_id      设备编号                       */
/*		          : int status      布防/撤防                       */
/* Output         : none                                                      */
/* Return         : true   节点更新                                           */
/*                  false  节点增加                                           */
/******************************************************************************/
bool CDevInfoContainer::HashDeviceDefenceSet(char* device_id, int status)
{
	//更新设备明细表状态	
	HashDeviceStatusUpdate(device_id, SYS_ACTION_REMOVE_DEFENCE == status?DEVICE_NOT_USE:DEVICE_USE);

	//轮巡表撤防
	HashedDeviceRoll::iterator itQuery;
	PDEVICE_INFO_POLL device_node;
	DBG(("HashDeviceDefenceSet device_id[%s] status[%d]\n", device_id, status));
	CTblLockRoll.lock();
	for (itQuery = m_HashTableRoll->begin(); itQuery != m_HashTableRoll->end(); itQuery++)
	{
		device_node = (PDEVICE_INFO_POLL)(&itQuery->second);

		if (0 != strncmp(device_node->Id, device_id, DEVICE_ID_LEN))
		{
			continue;
		}
		if( status == SYS_ACTION_SET_DEFENCE )//布防
		{
			device_node->offLineCnt = device_node->offLineTotle;
		}
	}
	CTblLockRoll.unlock();

	//发送撤防布防通知
	QUERY_INFO_CPM OnLineAct;
	memset((unsigned char*)&OnLineAct, 0, sizeof(QUERY_INFO_CPM));
	OnLineAct.Cmd = 0x02;
	memcpy(OnLineAct.DevId, device_id, DEVICE_ID_LEN);
	int deviceStatus = status;
	if (SYS_ACTION_REMOVE_DEFENCE == status)//撤防
	{
		deviceStatus = DEVICE_NOT_USE;
	}
	else
	{
		deviceStatus = DEVICE_USE;
	}
	sprintf(OnLineAct.Value, "%d", deviceStatus); 
	time_t tm_NowTime;
	time(&tm_NowTime);
	Time2Chars(&tm_NowTime, OnLineAct.Time);

	if(!SendMsg_Ex(g_ulHandleAnalyser, MUCB_DEV_ANALYSE, (char*)&OnLineAct, sizeof(QUERY_INFO_CPM)))
	{
		DBG(("CDevInfoContainer CheckOnLineStatus 1  SendMsg_Ex failed  handle[%d]\n", g_ulHandleAnalyser));
	}

	return true;
}

/******************************************************************************/
/* Function Name  : HashDeviceStatusUpdate                                                */
/* Description    : 设备布防撤防                                        */
/* Input          : char* device_id      设备编号                       */
/*		          : int status			在线/离线                       */
/* Output         : none                                                      */
/* Return         : true   节点更新                                           */
/*                  false  节点增加                                           */
/******************************************************************************/
bool CDevInfoContainer::HashDeviceStatusUpdate(char* device_id, int status)
{
	bool ret = false;
	//更新设备明细表状态
	DEVICE_DETAIL_INFO stDetail;
	memset((unsigned char*)&stDetail, 0, sizeof(DEVICE_DETAIL_INFO));
	memcpy(stDetail.Id, device_id, DEVICE_ID_LEN);
	CTblLockDevice.lock();
	HashedDeviceDetail::iterator it = m_HashTableDevice->find(stDetail.Id);
	if (m_HashTableDevice->end() != it)
	{
		PDEVICE_DETAIL_INFO pNode = (PDEVICE_DETAIL_INFO)(&it->second);
		pNode->cStatus = status;
		ret = true;
	}
	CTblLockDevice.unlock();
	return ret;
}
int CDevInfoContainer::GetActionNode(char* deviceActionId, DEVICE_INFO_ACT& stPollNode)
{
	int ret = SYS_STATUS_DEVICE_ACTION_NOT_EXIST;
	memset((unsigned char*)&stPollNode, 0, sizeof(DEVICE_INFO_ACT));
	strcpy(stPollNode.Id, deviceActionId);
	CLockAction.lock();
	HashedDeviceAction::iterator it = m_HashTableAction->find(stPollNode.Id);
	if (m_HashTableAction->end() != it)
	{
		//执行动作
		PDEVICE_INFO_ACT pNode = (PDEVICE_INFO_ACT)(&it->second);
		memcpy((unsigned char*)&stPollNode, (unsigned char*)pNode, sizeof(DEVICE_INFO_ACT));
		ret = SYS_STATUS_SUCCESS;
	}
	CLockAction.unlock();
	return ret;
}

int CDevInfoContainer::GetNextPollAttrNode(const int iChannelId, const char* pDevAttrId, DEVICE_INFO_POLL& stNewNode)
{
	int ret = SYS_STATUS_FAILED;
	memset((unsigned char*)&stNewNode, 0, sizeof(DEVICE_INFO_POLL));
	strcpy(stNewNode.Id, pDevAttrId);

	if (DEVICE_ID_LEN + DEVICE_SN_LEN != strlen(pDevAttrId))//设备编号不合法，则查找对应通道的第一个属性
	{
		return GetFirstPollAttrNode(iChannelId, stNewNode);
	}

	CTblLockRoll.lock();
	HashedDeviceRoll::iterator it = m_HashTableRoll->find(stNewNode.Id);
	if (m_HashTableRoll->end() != it)
	{
		//执行动作
		PDEVICE_INFO_POLL pNode = (PDEVICE_INFO_POLL)(&it->second);
		if (iChannelId == atoi(pNode->Upper_Channel)
			|| (atoi(pNode->Upper_Channel) >= INTERFACE_AD_A0 && atoi(pNode->Upper_Channel) <= INTERFACE_AV_OUT_1 
			&& iChannelId >= INTERFACE_AD_A0 && iChannelId <= INTERFACE_AV_OUT_1 ))
		{
			memcpy((unsigned char*)&stNewNode, (unsigned char*)pNode, sizeof(DEVICE_INFO_POLL));
			ret = SYS_STATUS_SUCCESS;
		}
	}
	CTblLockRoll.unlock();

	if (SYS_STATUS_SUCCESS != ret)
	{
		return GetFirstPollAttrNode(iChannelId, stNewNode);
	}
	return ret;
}

int CDevInfoContainer::GetFirstPollAttrNode(int iChannleId, DEVICE_INFO_POLL& stNewNode)
{
	int ret = SYS_STATUS_FAILED;
	HashedDeviceRoll::iterator it;
	PDEVICE_INFO_POLL device_node;
	
	CTblLockRoll.lock();
	for (it = m_HashTableRoll->begin(); it != m_HashTableRoll->end(); it++)
	{
		device_node = (PDEVICE_INFO_POLL)(&it->second);
		if (iChannleId == atoi(device_node->Upper_Channel)
			|| (atoi(device_node->Upper_Channel) >= INTERFACE_AD_A0 && atoi(device_node->Upper_Channel) <= INTERFACE_AV_OUT_1 
				&& iChannleId >= INTERFACE_AD_A0 && iChannleId <= INTERFACE_AV_OUT_1 ))
		{
			memcpy((unsigned char*)&stNewNode, (unsigned char*)device_node, sizeof(DEVICE_INFO_POLL));
			ret = SYS_STATUS_SUCCESS;
			break;
		}
	}
	CTblLockRoll.unlock();	
	return ret;
}

int CDevInfoContainer::GetDeviceRollByChannelId(int iChannel, DEVICE_INFO_POLL& stPollNode)
{
	int ret = SYS_STATUS_FAILED;
	HashedDeviceRoll::iterator it;
	CTblLockRoll.lock();

	for (it = m_HashTableRoll->begin(); it != m_HashTableRoll->end(); it++)
	{
		PDEVICE_INFO_POLL device_node = (PDEVICE_INFO_POLL)(&it->second);
		if (iChannel == atoi(device_node->Upper_Channel))
		{
			memcpy((unsigned char*)&stPollNode, (unsigned char*)device_node, sizeof(DEVICE_INFO_POLL));
			ret = SYS_STATUS_SUCCESS;
			break;
		}
	}
	CTblLockRoll.unlock();
	return ret;
}

int CDevInfoContainer::GetDeviceRollByDevId(char* devId, DEVICE_INFO_POLL& stPollNode)
{
	int ret = SYS_STATUS_FAILED;
	HashedDeviceRoll::iterator it;
	CTblLockRoll.lock();
	for (it = m_HashTableRoll->begin(); it != m_HashTableRoll->end(); it++)
	{
		PDEVICE_INFO_POLL device_node = (PDEVICE_INFO_POLL)(&it->second);
		if (0 == strncmp(device_node->Id, devId, strlen(devId)))
		{
			memcpy((unsigned char*)&stPollNode, (unsigned char*)device_node, sizeof(DEVICE_INFO_POLL));
			ret = SYS_STATUS_SUCCESS;
			break;
		}
	}
	CTblLockRoll.unlock();
	return ret;
}


int CDevInfoContainer::GetDeviceRollBySpecialDevTypeId(char* deviceType, char* attrId, DEVICE_INFO_POLL& stPollNode)
{
	int ret = SYS_STATUS_FAILED;
	HashedDeviceRoll::iterator it;
	CTblLockRoll.lock();

	for (it = m_HashTableRoll->begin(); it != m_HashTableRoll->end(); it++)
	{
		PDEVICE_INFO_POLL device_node = (PDEVICE_INFO_POLL)(&it->second);

		if (0 == strncmp(device_node->Id, deviceType, strlen(deviceType))
			&& 0 == strncmp(device_node->Id + DEVICE_TYPE_LEN, "0001", 4)			
			&& 0 == strncmp(device_node->Id + DEVICE_ID_LEN, attrId, strlen(attrId))
			)
		{
			memcpy((unsigned char*)&stPollNode, (unsigned char*)device_node, sizeof(DEVICE_INFO_POLL));
			ret = SYS_STATUS_SUCCESS;
			break;
		}
	}
	CTblLockRoll.unlock();
	return ret;
}

int CDevInfoContainer::GetDeviceRollByChannelDevTypeId(int iChannel, char* deviceType, char* attrId, DEVICE_INFO_POLL& stPollNode)
{
	int ret = SYS_STATUS_FAILED;
	HashedDeviceRoll::iterator it;
	CTblLockRoll.lock();

	for (it = m_HashTableRoll->begin(); it != m_HashTableRoll->end(); it++)
	{
		PDEVICE_INFO_POLL device_node = (PDEVICE_INFO_POLL)(&it->second);

		if (iChannel == atoi(device_node->Upper_Channel)
			&& 0 == strncmp(device_node->Id, deviceType, strlen(deviceType))
			&& 0 == strncmp(device_node->Id + DEVICE_ID_LEN, attrId, strlen(attrId))
			)
		{
			memcpy((unsigned char*)&stPollNode, (unsigned char*)device_node, sizeof(DEVICE_INFO_POLL));
			ret = SYS_STATUS_SUCCESS;
			break;
		}
	}
	CTblLockRoll.unlock();
	return ret;
}
int CDevInfoContainer::DisPatchAction(ACTION_MSG stActMsgPara)
{
	int ret = SYS_STATUS_FAILED;
	char szActionId[DEVICE_ACT_ID_LEN_TEMP] = {0};

	ACTION_MSG stActMsg;
	memset(&stActMsg, 0, sizeof(ACTION_MSG));
	memcpy((unsigned char*)&stActMsg, (unsigned char*)&stActMsgPara, sizeof(ACTION_MSG));
	memcpy(szActionId, stActMsg.DstId, DEVICE_ID_LEN);
	memcpy(szActionId + DEVICE_ID_LEN, stActMsg.ActionSn, DEVICE_ACTION_SN_LEN);

	DEVICE_INFO_ACT stDeviceNode;
	if(SYS_STATUS_SUCCESS == g_DevInfoContainer.GetActionNode(szActionId, stDeviceNode))
	{
		if (0 == strcmp(ACTION_SET_DEFENCE, stActMsg.ActionSn) || 0 == strcmp(ACTION_REMOVE_DEFENCE, stActMsg.ActionSn))//布防、撤防动作
		{
			int status = 0;
			if(0 == strcmp(ACTION_SET_DEFENCE, stActMsg.ActionSn))
			{
				status = SYS_ACTION_SET_DEFENCE;
			}
			else
			{
				status = SYS_ACTION_REMOVE_DEFENCE;
			}
			if(g_DevInfoContainer.HashDeviceDefenceSet(stActMsg.DstId, status))
				ret = SYS_STATUS_SUCCESS;
		}
		else
		{
			
			DEVICE_DETAIL_INFO stDeviceDetail;
			memset((unsigned char*)&stDeviceDetail, 0, sizeof(DEVICE_DETAIL_INFO));
			memcpy(stDeviceDetail.Id, szActionId, DEVICE_ID_LEN);
			if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetDeviceDetailNode(stDeviceDetail))//获取设备
			{
				return SYS_STATUS_DEVICE_NOT_EXIST;
			}
			if(DEVICE_NOT_USE == stDeviceDetail.cStatus)//设备已撤防
			{
				return SYS_STATUS_DEVICE_REMOVE_DEFENCE;
			}

			if ('0' == stDeviceNode.cBeOn)//动作未启用
			{
				return SYS_STATUS_DEVICE_ACTION_NOT_USE;
			}

			char vActionId[DEVICE_ACT_ID_LEN_TEMP] = {0};
			strcpy(vActionId, stDeviceNode.szVActionId);
			if(strlen(vActionId) == (DEVICE_ID_LEN + DEVICE_ACTION_SN_LEN))//是虚拟动作
			{
				if(SYS_STATUS_SUCCESS == g_DevInfoContainer.GetActionNode(vActionId, stDeviceNode))//获取实体动作结点
				{
					//重组actionMgs
					memcpy(stActMsg.DstId, stDeviceNode.Id, DEVICE_ID_LEN);
					memcpy(stActMsg.ActionSn, stDeviceNode.Id + DEVICE_ID_LEN, DEVICE_ACTION_SN_LEN);
				}
				else
				{
					ret = SYS_STATUS_DEVICE_ACTION_NOT_EXIST;
					return ret;
				}
			}

			if (0 == strcmp(stDeviceNode.ctype2, "01"))//情景模式批量动作处理
			{
				QUERY_INFO_CPM stQueryInfo;
				memset((unsigned char*)&stQueryInfo, 0, sizeof(QUERY_INFO_CPM));
				stQueryInfo.Cmd = 4;
				memcpy(stQueryInfo.DevId, stActMsg.DstId, DEVICE_ID_LEN);
				strcpy(stQueryInfo.DevId + DEVICE_ID_LEN, "0001");
				stQueryInfo.VType = FLOAT;
				strcpy(stQueryInfo.Value, stActMsg.ActionSn);
				if(SendMsg_Ex(g_ulHandleAnalyser, MUCB_DEV_ANALYSE, (char*)&stQueryInfo, sizeof(QUERY_INFO_CPM)))
					ret = SYS_STATUS_SUBMIT_SUCCESS;
			}
			else
			{
				//是否需要执行上一模式的动作			
				DEVICE_MODE_INFO stModeNode;
				memset((unsigned char*)&stModeNode, 0, sizeof(DEVICE_MODE_INFO));
				DEVICE_DETAIL_INFO stDevNode;
				memset((unsigned char*)&stDevNode, 0, sizeof(DEVICE_DETAIL_INFO));
							
				if('0' == stDeviceNode.flag2 && '1' == stDeviceNode.flag3)//无模式，执行上次模式的默认值
				{
					g_DevInfoContainer.UpdateDeviceModeNodeCurValue(stDeviceNode.Id, stDeviceNode.Id + DEVICE_ID_LEN, stDeviceNode.szCmdName);
				}
				else if('1' == stDeviceNode.flag2 && '0' == stDeviceNode.flag3)//有模式，改变当前模式，并设置上次的值为当前值
				{		
					memcpy(stModeNode.Id, stDeviceNode.Id, DEVICE_MODEID_LEN);
					if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetDeviceModeNode(stModeNode))//获取上次值
					{
						DBG(("CDevInfoContainer::DisPatchAction GetDeviceDetailNode(%s) Faield\n", stDevNode.Id));
						return SYS_STATUS_ACION_RESUME_CONFIG_ERROR;
					}
					char curModeId[DEVICE_ACTION_SN_LEN_TEMP] = {0};
					memcpy(curModeId, stModeNode.Id + DEVICE_ID_LEN, DEVICE_MODE_LEN);
					memcpy(curModeId + DEVICE_MODE_LEN, stModeNode.curActionId, DEVICE_CUR_ACTION_LEN);
					//更新当前模式和值
					if(SYS_STATUS_SUCCESS != g_DevInfoContainer.UpdateDeviceDetailMode(stDeviceNode.Id, curModeId))
					{
						DBG(("CDevInfoContainer::DisPatchAction GetDeviceDetailNode(%s) Faield\n", stDevNode.Id));
						return SYS_STATUS_ACION_RESUME_CONFIG_ERROR;
					}
				}
				else if('1' == stDeviceNode.flag2 && '1' == stDeviceNode.flag3)//有模式，改变当前模式，并执行上次的值为当前值
				{	
					//更新当前模式和值
					if(SYS_STATUS_SUCCESS != g_DevInfoContainer.UpdateDeviceDetailMode(stDeviceNode.Id, stDeviceNode.Id + DEVICE_ID_LEN))
					{
						DBG(("CDevInfoContainer::DisPatchAction GetDeviceDetailNode(%s) Faield\n", stDevNode.Id));
						return SYS_STATUS_ACION_RESUME_CONFIG_ERROR;
					}
					g_DevInfoContainer.UpdateDeviceModeNodeCurValue(stDeviceNode.Id, stDeviceNode.Id + DEVICE_ID_LEN, stDeviceNode.szCmdName);
				}
				
				//更新当前模式的值域

				switch(atoi(stDeviceNode.Upper_Channel))
				{
				case INTERFACE_RS485_1:        //485 1号口        *****************************************
					if (SendMsg_Ex(g_ulHandleActionList481_1, MUCB_DEV_485_1, (char*)&stActMsg, sizeof(ACTION_MSG)))
						ret = SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP;
					break;
				case INTERFACE_RS485_2:        //485 2号口        *  1号口  *  3号口  *  5号口  *  7号口  *
					if (SendMsg_Ex(g_ulHandleActionList481_2, MUCB_DEV_485_2, (char*)&stActMsg, sizeof(ACTION_MSG)))
						ret = SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP;
					break;
					//                 *****************************************
				case INTERFACE_RS485_3:       //485 3号口        *  2号口  *  4号口  *  6号口  *  8号口  *
					if (SendMsg_Ex(g_ulHandleActionList481_3, MUCB_DEV_485_3, (char*)&stActMsg, sizeof(ACTION_MSG)))
						ret = SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP;
					break;
				case INTERFACE_RS485_4:       //485 4号口        *****************************************
					if (SendMsg_Ex(g_ulHandleActionList481_4, MUCB_DEV_485_4, (char*)&stActMsg, sizeof(ACTION_MSG)))
						ret = SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP;
					break;
				case INTERFACE_RS485_5:       //485 5号口     485(1) ->ttyS3    485(8) ->SPI1.1 (serial0)   485(7) ->SPI1.2 (serial0)
					if (SendMsg_Ex(g_ulHandleActionList481_5, MUCB_DEV_485_5, (char*)&stActMsg, sizeof(ACTION_MSG)))
						ret = SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP;
					break;
				case INTERFACE_RS485_6:       //485 6号口     485(2) ->ttyS2    485(4) ->SPI1.1 (serial1)   485(3) ->SPI1.2 (serial1)
					if (SendMsg_Ex(g_ulHandleActionList481_6, MUCB_DEV_485_6, (char*)&stActMsg, sizeof(ACTION_MSG)))
						ret = SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP;
					break;
				case INTERFACE_RS485_7:       //485 7号口                       485(6) ->SPI1.1 (serial2)   485(5) ->SPI1.2 (serial2)
					if (SendMsg_Ex(g_ulHandleActionList481_7, MUCB_DEV_485_7, (char*)&stActMsg, sizeof(ACTION_MSG)))
						ret = SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP;
					break;
				case INTERFACE_RS485_8:       //485 8号口
					if (SendMsg_Ex(g_ulHandleActionList481_8, MUCB_DEV_485_8, (char*)&stActMsg, sizeof(ACTION_MSG)))
						ret = SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP;
					break;	
					//开关量输出上排
				case INTERFACE_DOUT_A2:
				case INTERFACE_DOUT_A4:
				case INTERFACE_DOUT_B2:
				case INTERFACE_DOUT_B4:

					//开关量输出下排
				case INTERFACE_DOUT_A1:
				case INTERFACE_DOUT_A3:
				case INTERFACE_DOUT_B1:
				case INTERFACE_DOUT_B3:
					//继电器电源输出
				case INTERFACE_AV_OUT_1:
					if (SendMsg_Ex(g_ulHandleActionListAD, MUCB_DEV_ADS, (char*)&stActMsg, sizeof(ACTION_MSG)))
						ret = SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP;
					break;	
				case INTERFACE_GSM:
					{
						//拆分手机号
						char szTempValue[1024] = {0};
						strcpy(szTempValue, stActMsg.ActionValue);

						//获取GSM内容
						char szSmsContent[1024] = {0};
						char * pValue = strstr(szTempValue, "//");
						if (NULL == pValue)
						{
							ret = SYS_STATUS_FORMAT_ERROR;//格式错误
							break;
						}
						strcpy(szSmsContent, pValue + 2);

						char szTelNumbers[256] = {0};
						strncpy(szTelNumbers, szTempValue, pValue - szTempValue);

						//获取目的手机号码
						char* savePtr = NULL;
						char* split_result = strtok_r(szTelNumbers, "/", &savePtr);
						while( split_result != NULL )
						{
							char newValue[1024] = {0};
							sprintf(newValue, "%s//%s", split_result, szSmsContent);

							GSM_MSG stGsmMsg;
							memset((unsigned char*)&stGsmMsg, 0, sizeof(GSM_MSG));
							stGsmMsg.iMsgType = 1;//需要回应
							memcpy((unsigned char*)&stGsmMsg.stAction, (unsigned char*)&stActMsg, sizeof(ACTION_MSG));
							stGsmMsg.stAction.ActionSource = ACTION_TO_GSM;
							memset(stGsmMsg.stAction.ActionValue, 0, sizeof(stGsmMsg.stAction.ActionValue));
							strcpy(stGsmMsg.stAction.ActionValue, newValue);

							DBG(("发送到GSM SrcId[%s] AttrId[%s] DstId[%s] ActionId[%s] ActionValue[%s]\n", stGsmMsg.stAction.SrcId, stGsmMsg.stAction.AttrSn, stGsmMsg.stAction.DstId, stGsmMsg.stAction.ActionSn, stGsmMsg.stAction.ActionValue));
							if (SendMsg_Ex(g_ulHandleGSM, MUCB_DEV_GSM, (char*)&stGsmMsg, sizeof(GSM_MSG)))
								ret = SYS_STATUS_SUBMIT_SUCCESS_IN_LIST;
							
							split_result = strtok_r(NULL, "/", &savePtr);
						}
					}
					break;
					//网络数据
				default:
					if (INTERFACE_NET_INPUT <= atoi(stDeviceNode.Upper_Channel))
					{
						//私有属性有 设备序列号、登录密码
						//发送到网络型传感器网关						
						ret = g_NetContainer.DoAction(stActMsg);
					}
					if (0 == strncmp(stActMsg.DstId, CPM_GATEWAY_ID, DEVICE_ID_LEN))//网关告警,上传至上级应用平台
					{
						DEVICE_DETAIL_INFO deviceInfo;
						memset((unsigned char*)&deviceInfo, 0, sizeof(DEVICE_DETAIL_INFO));
						memcpy(deviceInfo.Id, stActMsg.SrcId, DEVICE_ID_LEN);
						if (SYS_STATUS_SUCCESS == g_DevInfoContainer.GetDeviceDetailNode(deviceInfo))//获取设备信息
						{
							DEVICE_INFO_POLL devicePoll;
							char szAttrId[DEVICE_ATTR_ID_LEN_TEMP] = {0};
							strncpy(szAttrId, stActMsg.SrcId, DEVICE_ID_LEN);
							strncpy(szAttrId + DEVICE_ID_LEN, stActMsg.AttrSn, DEVICE_SN_LEN);
							if (SYS_STATUS_SUCCESS == g_DevInfoContainer.GetAttrNode(szAttrId, devicePoll))//获取属性信息
							{
								//上传至平台
								char updateData[MAXMSGSIZE] = {0};
								memset(updateData, 0, sizeof(updateData));
								char* ptrBuf = updateData;
								memcpy(ptrBuf, stActMsg.SrcId, DEVICE_ID_LEN);
								ptrBuf += DEVICE_ID_LEN;

								sprintf(ptrBuf, "%-30s", deviceInfo.szCName);
								ptrBuf += 30;

								strcpy(ptrBuf, stActMsg.AttrSn);
								ptrBuf += DEVICE_SN_LEN;

								sprintf(ptrBuf, "%-20s", devicePoll.szAttrName);
								ptrBuf += 20;

								sprintf(ptrBuf, "%d", ALARM_GENERAL);
								ptrBuf += 1;

								sprintf(ptrBuf, "%-20s", stActMsg.szActionTimeStamp);
								ptrBuf += 20;

								sprintf(ptrBuf, "%-128s", stActMsg.ActionValue);
								ptrBuf += 128;
								Net_Up_Msg(CMD_ALARM_UPLOAD, updateData, ptrBuf - updateData);
								ret = SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP;//网关联动时，不发送联动数据到平台，只有CAnalyser::DecodeAct用到
							}
						}
					}
					break;
				}
			}
		}
	}
	else
	{
		DBG((".GetActionNode(%s)failed\n", szActionId));
	}
	return ret;
}

/******************************************************************************/
/* Function Name  : SendToActionSource                                     */
/* Description    : 插入到设备动作回应表节点                                  */
/* Input          : ACTION_MSG actionMsg				                      */
/*		          : int actionRet				                      */
/* Output         : 无                                               */
/* Return         : 0	失败		                                              */
/*                  1   成功                                           */
/******************************************************************************/
void CDevInfoContainer::SendToActionSource(ACTION_MSG actionMsg, int actionRet)
{	
	time_t tm_NowTime;
	char updateData[1024] = {0};
	CDevInfoContainer::InsterIntoAlarmInfo(actionMsg, actionRet);
	switch(actionMsg.ActionSource)
	{
	case ACTION_SOURCE_ONTIME:
	case ACTION_SOURCE_SYSTEM://系统联动
	case ACTION_TO_GSM:
	case ACTION_RESUME:
		{
			//无需回应
			//上传数据到平台
			{
				char charsTime[22] = {0};

				time(&tm_NowTime);
				Time2Chars(&tm_NowTime, charsTime);

				DEVICE_DETAIL_INFO srcDeviceInfo;
				memset((unsigned char*)&srcDeviceInfo, 0, sizeof(DEVICE_DETAIL_INFO));
				DEVICE_INFO_POLL srcDeviceAttr;
				memset((unsigned char*)&srcDeviceAttr, 0, sizeof(DEVICE_INFO_POLL));
				if (DEVICE_ID_LEN == strlen(trim(actionMsg.SrcId, strlen(actionMsg.SrcId))))
				{
					memcpy(srcDeviceInfo.Id, actionMsg.SrcId, DEVICE_ID_LEN);
					g_DevInfoContainer.GetDeviceDetailNode(srcDeviceInfo);//获取设备名
					if (DEVICE_SN_LEN == strlen(trim(actionMsg.AttrSn, strlen(actionMsg.AttrSn))))
					{
						memcpy(srcDeviceAttr.Id, actionMsg.SrcId, DEVICE_ID_LEN);
						memcpy(srcDeviceAttr.Id + DEVICE_ID_LEN, actionMsg.AttrSn, DEVICE_SN_LEN);
						g_DevInfoContainer.GetAttrNode(srcDeviceAttr.Id, srcDeviceAttr);//获取属性名
					}
				}

				DEVICE_DETAIL_INFO dstDeviceInfo;
				memset((unsigned char*)&dstDeviceInfo, 0, sizeof(DEVICE_DETAIL_INFO));
				DEVICE_INFO_ACT dstDeviceAct;
				memset((unsigned char*)&dstDeviceAct, 0, sizeof(DEVICE_INFO_ACT));
				if (DEVICE_ID_LEN == strlen(trim(actionMsg.DstId, strlen(actionMsg.DstId))))
				{
					memcpy(dstDeviceInfo.Id, actionMsg.DstId, DEVICE_ID_LEN);
					g_DevInfoContainer.GetDeviceDetailNode(dstDeviceInfo);//获取设备名
					if (DEVICE_ACTION_SN_LEN == strlen(trim(actionMsg.ActionSn, strlen(actionMsg.ActionSn))))
					{
						memcpy(dstDeviceAct.Id, actionMsg.DstId, DEVICE_ID_LEN);
						memcpy(dstDeviceAct.Id + DEVICE_ID_LEN, actionMsg.ActionSn, DEVICE_ACTION_SN_LEN);
						g_DevInfoContainer.GetDeviceActionNode(dstDeviceAct);//获取属性名
					}
				}

				char* ptrBuf = updateData;
				memcpy(ptrBuf, actionMsg.SrcId, DEVICE_ID_LEN);
				ptrBuf += DEVICE_ID_LEN;

				sprintf(ptrBuf, "%-30s", srcDeviceInfo.szCName);
				ptrBuf += 30;

				memcpy(ptrBuf, actionMsg.AttrSn, DEVICE_SN_LEN);
				ptrBuf += DEVICE_SN_LEN;

				sprintf(ptrBuf, "%-20s", srcDeviceAttr.szAttrName);
				ptrBuf += 20;

				sprintf(ptrBuf, "%-128s", actionMsg.SrcValue);
				ptrBuf += 128;
				
				memcpy(ptrBuf, actionMsg.DstId, DEVICE_ID_LEN);
				ptrBuf += DEVICE_ID_LEN;

				sprintf(ptrBuf, "%-30s", dstDeviceInfo.szCName);
				ptrBuf += 30;

				memcpy(ptrBuf, actionMsg.ActionSn, DEVICE_ACTION_SN_LEN);
				ptrBuf += DEVICE_ACTION_SN_LEN;

				sprintf(ptrBuf, "%-20s", dstDeviceAct.szCmdName);
				ptrBuf += 20;

				sprintf(ptrBuf, "%-256s", actionMsg.ActionValue);
				ptrBuf += 256;

				sprintf(ptrBuf, "%-20s", charsTime);
				ptrBuf += 20;

				sprintf(ptrBuf, "%-10s", actionMsg.Operator);
				ptrBuf += 10;

				sprintf(ptrBuf, "%04d", actionRet);
				ptrBuf += 4;

				Net_Up_Msg(CMD_ACT_UPLOAD, updateData, ptrBuf - updateData);
			}
		}
		break;
	case ACTION_SOURCE_NET_CPM://嵌入式指令
	case ACTION_SOURCE_NET_PLA://平台指令
		{
			//发送回应到动作请求源
			ACTION_RESP_MSG actionResp;
			actionResp.Seq = actionMsg.Seq;
			actionResp.Status = actionRet;
			printf("actionResp.Seq = actionMsg.Seq[%d]ret[%d]\n", actionResp.Seq, actionResp.Status);
			if (!SendMsg_Ex(g_ulHandleBoaMsgCtrl, MUCB_DEV_MSGCTRL_BOA, (char*)&actionResp, sizeof(ACTION_RESP_MSG)))
			{
				DBG(("发送回应到MsgCtrl SendMsg_Ex Failed: handle[%d] mucb[%d]\n", g_ulHandleBoaMsgCtrl, MUCB_DEV_MSGCTRL_BOA));
			}
			//上传数据到平台
			{
				char charsTime[22] = {0};
				time(&tm_NowTime);
				Time2Chars(&tm_NowTime, charsTime);

				DEVICE_DETAIL_INFO srcDeviceInfo;
				memset((unsigned char*)&srcDeviceInfo, 0, sizeof(DEVICE_DETAIL_INFO));
				DEVICE_INFO_POLL srcDeviceAttr;
				memset((unsigned char*)&srcDeviceAttr, 0, sizeof(DEVICE_INFO_POLL));
				if (DEVICE_ID_LEN == strlen(trim(actionMsg.SrcId, strlen(actionMsg.SrcId))))
				{
					memcpy(srcDeviceInfo.Id, actionMsg.SrcId, DEVICE_ID_LEN);
					g_DevInfoContainer.GetDeviceDetailNode(srcDeviceInfo);//获取设备名
					if (DEVICE_SN_LEN == strlen(trim(actionMsg.AttrSn, strlen(actionMsg.AttrSn))))
					{
						memcpy(srcDeviceAttr.Id, actionMsg.SrcId, DEVICE_ID_LEN);
						memcpy(srcDeviceAttr.Id + DEVICE_ID_LEN, actionMsg.AttrSn, DEVICE_SN_LEN);
						g_DevInfoContainer.GetAttrNode(srcDeviceAttr.Id, srcDeviceAttr);//获取属性名
					}
				}

				DEVICE_DETAIL_INFO dstDeviceInfo;
				memset((unsigned char*)&dstDeviceInfo, 0, sizeof(DEVICE_DETAIL_INFO));
				DEVICE_INFO_ACT dstDeviceAct;
				memset((unsigned char*)&dstDeviceAct, 0, sizeof(DEVICE_INFO_ACT));
				if (DEVICE_ID_LEN == strlen(trim(actionMsg.DstId, strlen(actionMsg.DstId))))
				{
					memcpy(dstDeviceInfo.Id, actionMsg.DstId, DEVICE_ID_LEN);
					g_DevInfoContainer.GetDeviceDetailNode(dstDeviceInfo);//获取设备名
					if (DEVICE_ACTION_SN_LEN == strlen(trim(actionMsg.ActionSn, strlen(actionMsg.ActionSn))))
					{
						memcpy(dstDeviceAct.Id, actionMsg.DstId, DEVICE_ID_LEN);
						memcpy(dstDeviceAct.Id + DEVICE_ID_LEN, actionMsg.ActionSn, DEVICE_ACTION_SN_LEN);
						g_DevInfoContainer.GetDeviceActionNode(dstDeviceAct);//获取属性名
					}
				}

				char* ptrBuf = updateData;
				char formatBuf[32] = {0};
				memset(formatBuf, 0, sizeof(formatBuf));
				sprintf(formatBuf, "%%-%ds", DEVICE_ID_LEN);
				sprintf(ptrBuf, formatBuf, actionMsg.SrcId);
				ptrBuf += DEVICE_ID_LEN;

				sprintf(ptrBuf, "%-30s", srcDeviceInfo.szCName);
				ptrBuf += 30;

				memset(formatBuf, 0, sizeof(formatBuf));
				sprintf(formatBuf, "%%-%ds", DEVICE_SN_LEN);
				sprintf(ptrBuf, formatBuf, actionMsg.AttrSn);
				ptrBuf += DEVICE_SN_LEN;
				
				sprintf(ptrBuf, "%-20s", srcDeviceAttr.szAttrName);
				ptrBuf += 20;

				sprintf(ptrBuf, "%-128s", actionMsg.SrcValue);
				ptrBuf += 128;
				
				memset(formatBuf, 0, sizeof(formatBuf));
				sprintf(formatBuf, "%%-%ds", DEVICE_ID_LEN);
				sprintf(ptrBuf, formatBuf, actionMsg.DstId);
				ptrBuf += DEVICE_ID_LEN;

				sprintf(ptrBuf, "%-30s", dstDeviceInfo.szCName);
				ptrBuf += 30;

				memset(formatBuf, 0, sizeof(formatBuf));
				sprintf(formatBuf, "%%-%ds", DEVICE_ACTION_SN_LEN);
				sprintf(ptrBuf, formatBuf, actionMsg.ActionSn);
				ptrBuf += DEVICE_ACTION_SN_LEN;

				sprintf(ptrBuf, "%-20s", dstDeviceAct.szCmdName);
				ptrBuf += 20;

				sprintf(ptrBuf, "%-256s", actionMsg.ActionValue);
				ptrBuf += 256;

				sprintf(ptrBuf, "%-20s", charsTime);
				ptrBuf += 20;

				sprintf(ptrBuf, "%-10s", actionMsg.Operator);
				ptrBuf += 10;

				sprintf(ptrBuf, "%04d", actionRet);
				ptrBuf += 4;

				Net_Up_Msg(CMD_ACT_UPLOAD, updateData, ptrBuf - updateData);
			}
		}
		break;
	case ACTION_SOURCE_GSM://短信指令
		{
			GSM_MSG gsmMsg;
			memset(&gsmMsg, 0, sizeof(GSM_MSG));
			gsmMsg.iMsgType = 3;				
			gsmMsg.stActionRespMsg.Seq = actionMsg.Seq;
			gsmMsg.stActionRespMsg.Status = actionRet;

			if (!SendMsg_Ex(g_ulHandleGSM, MUCB_DEV_GSM, (char*)&gsmMsg, sizeof(ACTION_RESP_MSG) + sizeof(int)))
			{
				DBG(("SendMsg_Ex Failed: handle[%d] mucb[%d]\n", g_ulHandleGSM, MUCB_DEV_SMS_SEND));
			}
			//上传数据到平台
			{
				char charsTime[22] = {0};
				time(&tm_NowTime);
				Time2Chars(&tm_NowTime, charsTime);

				DEVICE_DETAIL_INFO srcDeviceInfo;
				memset((unsigned char*)&srcDeviceInfo, 0, sizeof(DEVICE_DETAIL_INFO));
				DEVICE_INFO_POLL srcDeviceAttr;
				memset((unsigned char*)&srcDeviceAttr, 0, sizeof(DEVICE_INFO_POLL));
				if (DEVICE_ID_LEN == strlen(trim(actionMsg.SrcId, strlen(actionMsg.SrcId))))
				{
					memcpy(srcDeviceInfo.Id, actionMsg.SrcId, DEVICE_ID_LEN);
					g_DevInfoContainer.GetDeviceDetailNode(srcDeviceInfo);//获取设备名
					if (DEVICE_SN_LEN == strlen(trim(actionMsg.AttrSn, strlen(actionMsg.AttrSn))))
					{
						memcpy(srcDeviceAttr.Id, actionMsg.SrcId, DEVICE_ID_LEN);
						memcpy(srcDeviceAttr.Id + DEVICE_ID_LEN, actionMsg.AttrSn, DEVICE_SN_LEN);
						g_DevInfoContainer.GetAttrNode(srcDeviceAttr.Id, srcDeviceAttr);//获取属性名
					}
				}

				DEVICE_DETAIL_INFO dstDeviceInfo;
				memset((unsigned char*)&dstDeviceInfo, 0, sizeof(DEVICE_DETAIL_INFO));
				DEVICE_INFO_ACT dstDeviceAct;
				memset((unsigned char*)&dstDeviceAct, 0, sizeof(DEVICE_INFO_ACT));
				if (DEVICE_ID_LEN == strlen(trim(actionMsg.DstId, strlen(actionMsg.DstId))))
				{
					memcpy(dstDeviceInfo.Id, actionMsg.DstId, DEVICE_ID_LEN);
					g_DevInfoContainer.GetDeviceDetailNode(dstDeviceInfo);//获取设备名
					if (DEVICE_ACTION_SN_LEN == strlen(trim(actionMsg.ActionSn, strlen(actionMsg.ActionSn))))
					{
						memcpy(dstDeviceAct.Id, actionMsg.DstId, DEVICE_ID_LEN);
						memcpy(dstDeviceAct.Id + DEVICE_ID_LEN, actionMsg.ActionSn, DEVICE_ACTION_SN_LEN);
						g_DevInfoContainer.GetDeviceActionNode(dstDeviceAct);//获取属性名
					}
				}

				char* ptrBuf = updateData;
				
				char formatBuf[32] = {0};
				memset(formatBuf, 0, sizeof(formatBuf));
				sprintf(formatBuf, "%%-%ds", DEVICE_ID_LEN);
				sprintf(ptrBuf, formatBuf, actionMsg.SrcId);
				ptrBuf += DEVICE_ID_LEN;

				sprintf(ptrBuf, "%-30s", srcDeviceInfo.szCName);
				ptrBuf += 30;
				
				memset(formatBuf, 0, sizeof(formatBuf));
				sprintf(formatBuf, "%%-%ds", DEVICE_SN_LEN);
				sprintf(ptrBuf, formatBuf, actionMsg.AttrSn);
				ptrBuf += DEVICE_SN_LEN;

				sprintf(ptrBuf, "%-20s", srcDeviceAttr.szAttrName);
				ptrBuf += 20;

				sprintf(ptrBuf, "%-128s", actionMsg.SrcValue);
				ptrBuf += 128;
							
				memset(formatBuf, 0, sizeof(formatBuf));
				sprintf(formatBuf, "%%-%ds", DEVICE_ID_LEN);
				sprintf(ptrBuf, formatBuf, actionMsg.DstId);
				ptrBuf += DEVICE_ID_LEN;

				sprintf(ptrBuf, "%-30s", dstDeviceInfo.szCName);
				ptrBuf += 30;

				memset(formatBuf, 0, sizeof(formatBuf));
				sprintf(formatBuf, "%%-%ds", DEVICE_ACTION_SN_LEN);
				sprintf(ptrBuf, formatBuf, actionMsg.ActionSn);
				ptrBuf += DEVICE_ACTION_SN_LEN;

				sprintf(ptrBuf, "%-20s", dstDeviceAct.szCmdName);
				ptrBuf += 20;

				sprintf(ptrBuf, "%-256s", actionMsg.ActionValue);
				ptrBuf += 256;

				sprintf(ptrBuf, "%-20s", charsTime);
				ptrBuf += 20;

				sprintf(ptrBuf, "%-10s", actionMsg.Operator);
				ptrBuf += 10;

				sprintf(ptrBuf, "%04d", actionRet);
				ptrBuf += 4;

				Net_Up_Msg(CMD_ACT_UPLOAD, updateData, ptrBuf - updateData);
			}
		}
		break;
	}

}

//动作数据插入到alarm_info表
void CDevInfoContainer::InsterIntoAlarmInfo(ACTION_MSG actionMsg, int actRet)
{
	char sqlData[512] = {0};
	char szSn[DEVICE_SN_LEN_TEMP] = {0};
	char szStatus[5] = {0};

	strcat(sqlData, "null, '");
	strcat(sqlData, actionMsg.SrcId);
	strcat(sqlData, "', '");

	if (0 != strlen(actionMsg.AttrSn))
	{
		memcpy(szSn, actionMsg.AttrSn, DEVICE_SN_LEN);
	}

	strcat(sqlData, szSn);

	strcat(sqlData, "', '");
	strcat(sqlData, actionMsg.DstId);
	strcat(sqlData, "', '");
	strcat(sqlData, actionMsg.ActionSn);
	strcat(sqlData, "', ");
	strcat(sqlData, " datetime('now'), '");
	strcat(sqlData, actionMsg.ActionValue);
	strcat(sqlData, "', '0', '");
	strcat(sqlData, actionMsg.Operator);
	strcat(sqlData, "', '");
	sprintf(szStatus, "%04d", actRet);
	strcat(sqlData, szStatus);
	strcat(sqlData, "', '");
	strcat(sqlData, actionMsg.SrcValue);
	strcat(sqlData, "'");

	g_SqlCtl.insert_into_table("alarm_info", sqlData, true);
}
void CDevInfoContainer::DisPatchQueryAction(char* queryAttrId)
{
	CDeviceIfBase* ptrDeviceIf = NULL;

	DEVICE_INFO_POLL stDeviceNode;
	if(SYS_STATUS_SUCCESS == g_DevInfoContainer.GetAttrNode(queryAttrId, stDeviceNode))
	{
		switch(atoi(stDeviceNode.Upper_Channel))
		{
		case INTERFACE_RS485_1:        //485 1号口        *****************************************
			ptrDeviceIf = g_ptrDeviceIf485_1;
			break;
		case INTERFACE_RS485_2:        //485 2号口        *  1号口  *  3号口  *  5号口  *  7号口  *
			ptrDeviceIf = g_ptrDeviceIf485_2;
			break;
		case INTERFACE_RS485_3:       //485 3号口        *  2号口  *  4号口  *  6号口  *  8号口  *
			ptrDeviceIf = g_ptrDeviceIf485_3;
			break;
		case INTERFACE_RS485_4:       //485 4号口        *****************************************
			ptrDeviceIf = g_ptrDeviceIf485_4;
			break;
		case INTERFACE_RS485_5:       //485 5号口     485(1) ->ttyS3    485(8) ->SPI1.1 (serial0)   485(7) ->SPI1.2 (serial0)
			ptrDeviceIf = g_ptrDeviceIf485_5;
			break;
		case INTERFACE_RS485_6:       //485 6号口     485(2) ->ttyS2    485(4) ->SPI1.1 (serial1)   485(3) ->SPI1.2 (serial1)
			ptrDeviceIf = g_ptrDeviceIf485_6;
			break;
		case INTERFACE_RS485_7:       //485 7号口                       485(6) ->SPI1.1 (serial2)   485(5) ->SPI1.2 (serial2)
			ptrDeviceIf = g_ptrDeviceIf485_7;
			break;
		case INTERFACE_RS485_8:       //485 8号口
			ptrDeviceIf = g_ptrDeviceIf485_8;
			break;

			//模拟量输入
		case INTERFACE_AD_A0:
		case INTERFACE_AD_A1:
		case INTERFACE_AD_A2:
		case INTERFACE_AD_A3:
		case INTERFACE_AD_B0:
		case INTERFACE_AD_B1:
		case INTERFACE_AD_B2:
		case INTERFACE_AD_B3:
			//开关量输入
		case INTERFACE_DIN_A0:
		case INTERFACE_DIN_A1:
		case INTERFACE_DIN_A2:
		case INTERFACE_DIN_A3:
		case INTERFACE_DIN_B0:
		case INTERFACE_DIN_B1:
		case INTERFACE_DIN_B2:
		case INTERFACE_DIN_B3:
			//开关量输出上排
		case INTERFACE_DOUT_A2:
		case INTERFACE_DOUT_A4:
		case INTERFACE_DOUT_B2:
		case INTERFACE_DOUT_B4:
			//开关量输出下排
		case INTERFACE_DOUT_A1:
		case INTERFACE_DOUT_A3:
		case INTERFACE_DOUT_B1:
		case INTERFACE_DOUT_B3:

			//继电器电源输出
		case INTERFACE_AV_OUT_1:
			ptrDeviceIf = g_ptrDeviceIfAD;
			break;
		case INTERFACE_GSM:
			break;			
		default:	//网络数据
			if (INTERFACE_NET_INPUT <= atoi(stDeviceNode.Upper_Channel))
			{
				//私有属性有 设备序列号、登录密码
			}
			break;
		}
		if (NULL != ptrDeviceIf)
		{
			ptrDeviceIf->QueryAttrProc(stDeviceNode);
		}
	}
}

int CDevInfoContainer::GetDeviceActionNode(DEVICE_INFO_ACT& node)
{
	int ret = SYS_STATUS_FAILED;
	HashedDeviceAction::iterator it;
	CLockAction.lock();
	it = m_HashTableAction->find(node.Id);
	if (m_HashTableAction->end() != it)
	{
		PDEVICE_INFO_ACT pNode = (PDEVICE_INFO_ACT)(&it->second);
		memcpy((unsigned char*)&node, (unsigned char*)pNode, sizeof(DEVICE_INFO_ACT));
		ret = SYS_STATUS_SUCCESS;
	}
	CLockAction.unlock();
	return ret;
}
int CDevInfoContainer::AddDeviceAction(DEVICE_INFO_ACT node)
{
	int ret = SYS_STATUS_FAILED;
	return ret;
}
int CDevInfoContainer::UpdateDeviceAction(DEVICE_INFO_ACT node)
{
	bool ret = false;
	HashedDeviceAction::iterator it;
	CLockAction.lock();
	it = m_HashTableAction->find(node.Id);
	if (m_HashTableAction->end() != it)
	{
		m_HashTableAction->erase(it);
	}
	ret = true;
	m_HashTableAction->insert(HashedDeviceAction::value_type(node.Id,node));
	CLockAction.unlock();
	return ret;
}
int CDevInfoContainer::DeleteDeviceAction(DEVICE_INFO_ACT node)
{
	int ret = -1;
	return ret;
}
int CDevInfoContainer::UpdateDeviceActionStatus(char* deviceId, char* deviceSn, char cBeOn, char* actName, char* vDevId, char* vActionId)
{
	bool ret = false;
	HashedDeviceAction::iterator it;
	CLockAction.lock();
	DEVICE_INFO_ACT node;
	memset((unsigned char*)&node, 0, sizeof(DEVICE_INFO_ACT));
	strncpy(node.Id, deviceId, DEVICE_ID_LEN);
	strncpy(node.Id + DEVICE_ID_LEN, deviceSn, DEVICE_ACTION_SN_LEN);
	it = m_HashTableAction->find(node.Id);
	if (m_HashTableAction->end() != it)
	{
		PDEVICE_INFO_ACT pNode = (PDEVICE_INFO_ACT)(&it->second);
		pNode->cBeOn = cBeOn;
		memset(pNode->szCmdName, 0, sizeof(pNode->szCmdName));
		strcpy(pNode->szCmdName, actName);//lhy，检查是否更新成功
		memcpy(pNode->szVActionId, vDevId, DEVICE_ID_LEN);
		memcpy(pNode->szVActionId + DEVICE_ID_LEN, vActionId, DEVICE_ACTION_SN_LEN);
		ret = true;
	}
	CLockAction.unlock();
	return ret;
}

int CDevInfoContainer::InitDeviceIndex(DeviceInit_CallBack FuncSelectCallBack, void * userData)
{
	int ret = -1;
	HashedDeviceRoll::iterator itQuery;
	PDEVICE_INFO_POLL device_node;
	int i = 0;
	CTblLockRoll.lock();
	for (itQuery = m_HashTableRoll->begin(); itQuery != m_HashTableRoll->end(); itQuery++)
	{
		device_node = (PDEVICE_INFO_POLL)(&itQuery->second);
		
		{
			DBG(("InitDeviceIndex[%d] channelId[%s] id[%s] preId[%s] nextId[%s]\n", i++, device_node->Upper_Channel, device_node->Id, device_node->PreId, device_node->NextId));
			FuncSelectCallBack(userData, device_node);
		}
	}
	CTblLockRoll.unlock();
	return ret;
}

/******************************************************************************/
/* Function Name  : ActionHashUpdateByNet                                                */
/* Description    : 更新设备动作表节点                                        */
/* Input          : NET_DEVICE_INFO_UPDATE new_node      设备节点                       */
/* Output         : none                                                      */
/* Return         : true   节点更新                                           */
/*                  false  节点增加                                           */
/******************************************************************************/
bool CDevInfoContainer::ActionHashUpdateByNet(NET_DEVICE_INFO_UPDATE new_node)
{
	bool ret = false;
	HashedDeviceAction::iterator it;
	PDEVICE_INFO_ACT device_node;
	CLockAction.lock();

	for (it = m_HashTableAction->begin(); it != m_HashTableAction->end(); it++)
	{
		device_node = (PDEVICE_INFO_ACT)(&it->second);

		if (0 != strncmp(device_node->Id, new_node.Id, DEVICE_ID_LEN))
		{
			continue;
		}

		memset(device_node->Upper_Channel, 0, sizeof(device_node->Upper_Channel));
		memset(device_node->Self_Id, 0, sizeof(device_node->Self_Id));

		strcpy(device_node->Upper_Channel, new_node.Upper_Channel);
		strcpy(device_node->Self_Id, new_node.Self_Id);
		LabelIF_GetDeviceActionPrivatePara(new_node.PrivateAttr, device_node);
	}
	ret = true;
	CLockAction.unlock();
	return ret;
}
/******************************************************************************/
/* Function Name  : ActionHashDeleteByNet                                                */
/* Description    : 删除设备动作表节点                                        */
/* Input          : NET_DEVICE_INFO_UPDATE new_node      设备节点                       */
/* Output         : none                                                      */
/* Return         : true   节点更新                                           */
/*                  false  节点增加                                           */
/******************************************************************************/
int CDevInfoContainer::ActionHashDeleteByNet(NET_DEVICE_INFO_UPDATE new_node)
{
	int ret = SYS_STATUS_FAILED;
	HashedDeviceAction::iterator it;
	PDEVICE_INFO_ACT device_node;
	DEVICE_INFO_ACT oldNodes[256];
	memset((unsigned char*)&oldNodes[0], 0, 256*sizeof(DEVICE_INFO_ACT));

	int startNodeCount = 0;

	CLockAction.lock();

	for (it = m_HashTableAction->begin(); it != m_HashTableAction->end(); it++)
	{
		device_node = (PDEVICE_INFO_ACT)(&it->second);

		if (0 == strncmp(device_node->Id, new_node.Id, DEVICE_ID_LEN))
		{
			memcpy((unsigned char*)&oldNodes[startNodeCount++], (unsigned char*)device_node, sizeof(DEVICE_INFO_ACT));
		}
	}
	for(int i=0; i<startNodeCount; i++)
	{
		delNodeAct(oldNodes[i]);
	}
	ret = SYS_STATUS_SUCCESS;
	CLockAction.unlock();
	return ret;
}

/******************************************************************************/
/* Function Name  : delNode                                                   */
/* Description    : 从设备表里删除节点                                        */
/* Input          : DEVICE_INFO_ACT hash_node      设备节点                   */
/* Output         : none                                                      */
/* Return         : true   节点删除成功                                       */
/*                  false  节点删除失败                                       */
/******************************************************************************/
bool CDevInfoContainer::delNodeAct(DEVICE_INFO_ACT hash_node)
{
	HashedDeviceAction::iterator it;
	it = m_HashTableAction->find(hash_node.Id);	
	if (it != m_HashTableAction->end())
	{
		m_HashTableAction->erase(it);
	}
	return true;
}



/******************************************************************************/
/* Function Name  : SetDeviceLineStatus                                            */
/* Description    : 设置网络设备在线离线		                                      */
/* Input          : DEVICE_INFO_POLL *p_node	        轮巡结点      */
/*		          : int nLen							数据长度      */
/* Output         : none                   */
/* Return         : int ret            */
/******************************************************************************/
int CDevInfoContainer::SetDeviceLineStatus(char* deviceAttrId, bool bStatus)
{
	DBG(("CDevInfoContainer::SetDeviceLineStatus deviceAttrId[%s]\n", deviceAttrId));
	//	int ret = SYS_STATUS_FAILED;
	time_t tm_NowTime;
	time(&tm_NowTime);    
	QUERY_INFO_CPM stOnlinNotice;

	//根据bStatus
	//如果为false
	//修改所有属性值为NULL，发出离线通知
	//修改设备状态为离线
	/*
	HashedDeviceRoll::iterator it;
	CTblLockRoll.lock();
	it = m_HashTableRoll->find(hash_node);	
	if (it._M_cur != NULL)
	{
	m_HashTableRoll->erase(it);
	}
	*/

	HashedDeviceRoll::iterator it;
	if(!bStatus)//设备离线,设置属性值为空
	{
		CTblLockRoll.lock();
		for (it = m_HashTableRoll->begin(); it != m_HashTableRoll->end(); it++)
		{
			PDEVICE_INFO_POLL device_node = (PDEVICE_INFO_POLL)(&it->second);
			if (0 == strncmp(device_node->Id, deviceAttrId, 14))
			{
				memset((unsigned char*)&stOnlinNotice, 0, sizeof(QUERY_INFO_CPM));

				device_node->offLineCnt = 0;	
				device_node->stLastValueInfo.DataStatus = 1;

				memcpy(stOnlinNotice.DevId, device_node->Id, DEVICE_ATTR_ID_LEN);
				stOnlinNotice.Cmd = 0;
				memcpy(stOnlinNotice.Type, device_node->stLastValueInfo.Type, 4);	
				stOnlinNotice.VType = CHARS;
				memset(stOnlinNotice.Value, 0, sizeof(stOnlinNotice.Value));
				strcpy(stOnlinNotice.Value, "NULL");
				stOnlinNotice.DataStatus = 1;			
				Time2Chars(&tm_NowTime, stOnlinNotice.Time);
				DBG(("SetDeviceLineStatus [%s] 属性离线2\n", stOnlinNotice.DevId));

				if(!SendMsg_Ex(g_ulHandleAnalyser, MUCB_DEV_ANALYSE, (char*)&stOnlinNotice, sizeof(QUERY_INFO_CPM)))
				{
					DBG(("SetDeviceLineStatus failed  handle[%d]\n", g_ulHandleAnalyser));
				}
			}
		}

		CTblLockRoll.unlock();	
		memset((unsigned char*)&stOnlinNotice, 0, sizeof(QUERY_INFO_CPM));

		//发送设备离线通知
		if (DEVICE_ID_LEN + 4 == strlen(deviceAttrId))
		{
			memcpy(stOnlinNotice.DevId, deviceAttrId, DEVICE_ID_LEN);
			strncpy(stOnlinNotice.DevId + DEVICE_ID_LEN, "0000", DEVICE_SN_LEN);
			memset(stOnlinNotice.Value, 0, sizeof(stOnlinNotice.Value));
			sprintf(stOnlinNotice.Value, "%d", DEVICE_OFFLINE);
			Time2Chars(&tm_NowTime, stOnlinNotice.Time);
			stOnlinNotice.Cmd = 1;
			if(!SendMsg_Ex(g_ulHandleAnalyser, MUCB_DEV_ANALYSE, (char*)&stOnlinNotice, sizeof(QUERY_INFO_CPM)))
			{
				DBG(("SetDeviceLineStatus SendMsg_Ex failed  handle[%d]\n", g_ulHandleAnalyser));
			}
		}
	}
	else
	{

		CTblLockRoll.lock();
		for (it = m_HashTableRoll->begin(); it != m_HashTableRoll->end(); it++)
		{
			PDEVICE_INFO_POLL device_node = (PDEVICE_INFO_POLL)(&it->second);
			if (0 == strncmp(device_node->Id, deviceAttrId, 14))
			{
				device_node->offLineCnt = device_node->offLineTotle;	
				device_node->stLastValueInfo.DataStatus = 0;
			}
		}

		CTblLockRoll.unlock();	

		memset((unsigned char*)&stOnlinNotice, 0, sizeof(QUERY_INFO_CPM));
		memcpy(stOnlinNotice.DevId, deviceAttrId, DEVICE_ID_LEN);
		stOnlinNotice.DevId[DEVICE_ID_LEN] = '\0';
		memset(stOnlinNotice.Value, 0, sizeof(stOnlinNotice.Value));
		sprintf(stOnlinNotice.Value, "%d", DEVICE_ONLINE);
		Time2Chars(&tm_NowTime, stOnlinNotice.Time);
		stOnlinNotice.Cmd = 1;	
		if(!SendMsg_Ex(g_ulHandleAnalyser, MUCB_DEV_ANALYSE, (char*)&stOnlinNotice, sizeof(QUERY_INFO_CPM)))
		{
			DBG(("SetDeviceLineStatus SendMsg_Ex failed  handle[%d]\n", g_ulHandleAnalyser));
		}
	}
	return SYS_STATUS_SUCCESS;
}

/*************************************  END OF FILE *****************************************/
