#include <stdio.h>
#include <locale.h>
#include <time.h>
 
#include "TkMacros.h"
#include "NetApp.h"
#include "Init.h"
#include "LabelIF.h"
#include "HardIF.h"


//设备状态查询


void Net_App_Resp_Proc(void* data, char* outBuf, int outLen,  unsigned int seq)
{
	MsgHdr *pRecMsgHdr = (MsgHdr *)data;

	char resp_msg[MAXMSGSIZE] = {0};
	MsgHdr *pRespMsgHdr = (MsgHdr *)resp_msg;
	char* pRespMsg = resp_msg + MSGHDRLEN;

	pRespMsgHdr->unMsgLen = MSGHDRLEN + outLen;
	pRespMsgHdr->unMsgCode = pRecMsgHdr->unMsgCode;
	pRespMsgHdr->unStatus = pRecMsgHdr->unStatus;
	pRespMsgHdr->unMsgSeq = seq;
	pRespMsgHdr->unRspTnlId = pRecMsgHdr->unRspTnlId;

	memcpy(pRespMsg, outBuf, outLen);
	DBG(("Len:[%d]  Status:[%d]  Seq:[%d]  TnlId:[%d]\n[%d][%s]\n", pRespMsgHdr->unMsgLen, pRespMsgHdr->unStatus, pRespMsgHdr->unMsgSeq, pRespMsgHdr->unRspTnlId, g_ulHandleNetBoaSvrSend, pRespMsg));

	if (!SendMsg(g_ulHandleNetBoaSvrSend, MUCB_IF_NET_SVR_SEND_BOA, resp_msg))
	{
		DBG(("doSVR_Submit_Func SendMsg_Ex Failed; handle[%d]\n", g_ulHandleNetBoaSvrSend));
	}
}

int Net_Dev_Status_Query(void * msg)
{
	int ret = SYS_STATUS_FAILED_NO_RESP; 

	//业务包指针
	PStrNetDevStatusQuery pRecMsg = (PStrNetDevStatusQuery)((char*)msg + MSGHDRLEN);
	char outBuf[MAXMSGSIZE] = {0};
	memcpy(outBuf, (char*)msg + MSGHDRLEN, 28);

	char szDeviceIds[1025] = {0};
	if (1024 < strlen(pRecMsg->szIds))
	{
		DBG(("Net_Dev_Status_Query idlen[%zu]\n", strlen(pRecMsg->szIds)));
		return ret;
	}
	strcpy(szDeviceIds, pRecMsg->szIds);

	char* ptrtemp = NULL;
	char* ptrdate = NULL;
	char* buf = szDeviceIds;
	ptrdate = strtok_r(buf, ",", &ptrtemp);
	char szContent[128] = {0};
	int seq = 0;
	DBG(("Net_Dev_Status_Query 11111\n"));
	while(NULL != ptrdate)
	{
		char deviceId[DEVICE_ID_LEN_TEMP] = {0};
		memcpy(deviceId, ptrdate, DEVICE_ID_LEN);
		ptrdate = strtok_r(NULL, ",", &ptrtemp);
		do 
		{
			if (DEVICE_ID_LEN == strlen(deviceId))
			{
				memset(szContent, 0, sizeof(szContent));
				DEVICE_DETAIL_STATUS stDeviceInfo;
				memset((unsigned char*)&stDeviceInfo, 0, sizeof(DEVICE_DETAIL_STATUS));

				DBG(("Net_Dev_Status_Query 2222 deviceId[%s]\n", deviceId));
				if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetDeviceStatusInfo(deviceId, stDeviceInfo))
				{
					DBG(("Net_Dev_Status_Query get deviceId failed[%s]\n", deviceId));
					break;
				}

				DBG(("Net_Dev_Status_Query 3333\n"));
				memcpy(szContent, stDeviceInfo.Id, DEVICE_ID_LEN);
				strcat(szContent, ",");
				char temp[2] = {0};
				sprintf(temp, "%01d", stDeviceInfo.Status);
				strcat(szContent, temp);
				strcat(szContent, ",");
				strcat(szContent, stDeviceInfo.Show);
				strcat(szContent, ";");
				if ((MAXMSGSIZE - MSGHDRLEN) < strlen(szContent) + strlen(outBuf))
				{
					if (28 < strlen(outBuf))
					{
						memcpy(outBuf+20, "0001", 4);
						Net_App_Resp_Proc(msg, outBuf, strlen(outBuf), seq++);
					}

					memset(outBuf, 0, sizeof(outBuf));
					memcpy(outBuf, (char*)msg + MSGHDRLEN, 28);
					if ( (MAXMSGSIZE - MSGHDRLEN) > strlen(szContent) + strlen(outBuf))
					{
						strcat(outBuf, szContent);
					}
				}
				else
				{
					strcat(outBuf, szContent);
					DBG(("Net_Dev_Status_Query 8888 outbuf[%s] content[%s]\n", outBuf, szContent));
				}
			}
		} while (false);
	}
	if (28 <= strlen(outBuf))
	{
		memcpy(outBuf+20, "0000", 4);
		Net_App_Resp_Proc(msg, outBuf, strlen(outBuf), seq++);
	}
	return ret;
}

int Net_Dev_Paras_Query(void * msg)
{
	int ret = SYS_STATUS_FAILED_NO_RESP; 

	//业务包指针
	PStrNetDevParasQuery pRecMsg = (PStrNetDevParasQuery)((char*)msg + MSGHDRLEN);
	char outBuf[MAXMSGSIZE] = {0};
	memcpy(outBuf, (char*)msg + MSGHDRLEN, 28);

	char szDeviceIds[1025] = {0};
	if (1024 < strlen(pRecMsg->szIds))
	{
		return ret;
	}
	strcpy(szDeviceIds, pRecMsg->szIds);

	char* ptrtemp = NULL;
	char* ptrdate = NULL;
	char* buf = szDeviceIds;
	ptrdate = strtok_r(buf, ",", &ptrtemp);
	int seq = 0;
	while(NULL != ptrdate)
	{
		do 
		{
			if (DEVICE_ID_LEN == strlen(ptrdate))
			{
				DEVICE_DETAIL_PARAS stDeviceInfo;
				memset((unsigned char*)&stDeviceInfo, 0, sizeof(DEVICE_DETAIL_PARAS));

				char deviceId[DEVICE_ID_LEN_TEMP] = {0};
				memcpy(deviceId, ptrdate, DEVICE_ID_LEN);
				if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetDeviceParasInfo(deviceId, stDeviceInfo, msg, seq))
					break;

				if ((MAXMSGSIZE - MSGHDRLEN) < strlen(stDeviceInfo.ParaValues) + strlen(outBuf))
				{
					memcpy(outBuf+20, "0001", 4);
					Net_App_Resp_Proc(msg, outBuf, strlen(outBuf), seq++);
					memset(outBuf, 0, sizeof(outBuf));
					memcpy(outBuf, (char*)msg + MSGHDRLEN, 28);
					if ( (MAXMSGSIZE - MSGHDRLEN) > strlen(stDeviceInfo.ParaValues) + strlen(outBuf))
					{
						strcat(outBuf, stDeviceInfo.ParaValues);
					}
				}
				else
				{
					strcat(outBuf, stDeviceInfo.ParaValues);
				}
			}
		} while (false);
		ptrdate = strtok_r(NULL, ",", &ptrtemp);
	}
	if (28 <= strlen(outBuf))
	{
		memcpy(outBuf+20, "0000", 4);
		Net_App_Resp_Proc(msg, outBuf, strlen(outBuf), seq++);
	}
	return ret;
}

int Net_Dev_Modes_Query(void * msg)
{
	int ret = SYS_STATUS_FAILED_NO_RESP; 

	//业务包指针
	PStrNetDevModesQuery pRecMsg = (PStrNetDevModesQuery)((char*)msg + MSGHDRLEN);
	char outBuf[MAXMSGSIZE] = {0};
	memcpy(outBuf, (char*)msg + MSGHDRLEN, 28);

	char szDeviceIds[1025] = {0};
	if (1024 < strlen(pRecMsg->szIds))
	{
		return ret;
	}
	strcpy(szDeviceIds, pRecMsg->szIds);

	char* ptrtemp = NULL;
	char* ptrdate = NULL;
	char* buf = szDeviceIds;
	ptrdate = strtok_r(buf, ",", &ptrtemp);
	int seq = 0;
	while(NULL != ptrdate)
	{
		do 
		{
			
			if(DEVICE_ID_LEN != strlen(ptrdate))
				break;
			
			DBG(("Net_Dev_Modes_Query ......3333 ptrdate[%s]\n", ptrdate));
			DEVICE_DETAIL_MODES stDeviceInfo;
			memset((unsigned char*)&stDeviceInfo, 0, sizeof(DEVICE_DETAIL_MODES));

			char deviceId[DEVICE_ID_LEN_TEMP] = {0};
			memcpy(deviceId, ptrdate, DEVICE_ID_LEN);
			if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetDeviceModesInfo(deviceId, stDeviceInfo, msg, seq))
			{
				
	DBG(("Net_Dev_Modes_Query ......4444 ptrdate[%s]\n", ptrdate));
				break;
			}
			
	DBG(("Net_Dev_Modes_Query ......5555 ptrdate[%s]\n", ptrdate));

			if ((MAXMSGSIZE - MSGHDRLEN) < strlen(stDeviceInfo.ParaValues) + strlen(outBuf))
			{
				memcpy(outBuf+20, "0001", 4);
				Net_App_Resp_Proc(msg, outBuf, strlen(outBuf), seq++);
				memset(outBuf, 0, sizeof(outBuf));
				memcpy(outBuf, (char*)msg + MSGHDRLEN, 28);
				if ( (MAXMSGSIZE - MSGHDRLEN) > strlen(stDeviceInfo.ParaValues) + strlen(outBuf))
				{
					strcat(outBuf, stDeviceInfo.ParaValues);
				}
			}
			else
			{
				strcat(outBuf, stDeviceInfo.ParaValues);
			}
		} while (false);
		ptrdate = strtok_r(NULL, ",", &ptrtemp);
	}
	if (28 <= strlen(outBuf))
	{
		memcpy(outBuf+20, "0000", 4);
		Net_App_Resp_Proc(msg, outBuf, strlen(outBuf), seq++);
	}
	return ret;
}
//设备状态查询
int Net_Dev_Query(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED; 

	//业务包指针
	PStrNetDevQuery pRecMsg = (PStrNetDevQuery)inBuf;

	memcpy(outBuf, inBuf, 28);

	char id[DEVICE_ID_LEN_TEMP] = {0};

	memcpy(id, pRecMsg->szDevType, DEVICE_ID_LEN);

	do 
	{		
		const char* sqlFormat = NULL;
		char sql[512] = {0};

		//判断设备是否存在
		sqlFormat = "where id = '%s'";
		sprintf(sql, sqlFormat, id);

		char queryResult[128] = {0};
		if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"device_detail", (char*)"count(*)", sql, queryResult))
		{
			ret = SYS_STATUS_DEVICE_NOT_EXIST;
			break;
		}

		char* split_result = NULL;
		char* savePtr = NULL;
		split_result = strtok_r(queryResult, "#", &savePtr);
		if (NULL == split_result)
		{
			ret = SYS_STATUS_DEVICE_NOT_EXIST;
			break;
		}

		if (1 != atoi(split_result))
		{
			ret = SYS_STATUS_DEVICE_NOT_EXIST;
			break;
		}

		DEVICE_DETAIL stDeviceInfo;
		memset((unsigned char*)&stDeviceInfo, 0, sizeof(DEVICE_DETAIL));
		outLen = 28;
		if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetDeviceInfoNode(id, stDeviceInfo))
			break;

		char szContent[1512] = {0};
		memcpy(szContent, stDeviceInfo.Id, DEVICE_ID_LEN);
		char temp[2] = {0};
		sprintf(temp, "%01d", stDeviceInfo.Status);
		memcpy(szContent+DEVICE_ID_LEN, temp, 1);
		memcpy(szContent + DEVICE_ID_LEN + 1, stDeviceInfo.Show, 56);
		strcpy(szContent + DEVICE_ID_LEN + 1 + 56, stDeviceInfo.ParaValues);
		int contentLen = DEVICE_ID_LEN + 1 + 56 + strlen(stDeviceInfo.ParaValues);
		outLen += contentLen;
		memcpy(outBuf + 28, szContent, contentLen);

		ret = SYS_STATUS_SUCCESS;
	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	return ret;
}
//设备实时状态查询
int Net_Dev_Real_Status_Query(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED; 

	//业务包指针
	PStrNetDevQuery pRecMsg = (PStrNetDevQuery)inBuf;

	memcpy(outBuf, inBuf, 28);

	char id[DEVICE_ID_LEN_TEMP] = {0};

	memcpy(id, pRecMsg->szDevType, DEVICE_ID_LEN);

	do 
	{		
		const char* sqlFormat = NULL;
		char sql[512] = {0};

		//判断设备是否存在
		sqlFormat = "where id = '%s'";
		sprintf(sql, sqlFormat, id);

		char queryResult[128] = {0};
		if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"device_detail", (char*)"count(*)", sql, queryResult))
		{
			ret = SYS_STATUS_DEVICE_NOT_EXIST;
			break;
		}

		char* split_result = NULL;
		char* savePtr = NULL;
		split_result = strtok_r(queryResult, "#", &savePtr);
		if (NULL == split_result)
		{
			ret = SYS_STATUS_DEVICE_NOT_EXIST;
			break;
		}

		if (1 != atoi(split_result))
		{
			ret = SYS_STATUS_DEVICE_NOT_EXIST;
			break;
		}

		DEVICE_DETAIL stDeviceInfo;
		memset((unsigned char*)&stDeviceInfo, 0, sizeof(DEVICE_DETAIL));
		outLen = 28;
		if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetDeviceRealInfo(id, stDeviceInfo))
			break;

		char szContent[1512] = {0};
		memcpy(szContent, stDeviceInfo.Id, DEVICE_ID_LEN);
		char temp[2] = {0};
		sprintf(temp, "%01d", stDeviceInfo.Status);
		memcpy(szContent+DEVICE_ID_LEN, temp, 1);
		memcpy(szContent + DEVICE_ID_LEN + 1, stDeviceInfo.Show, 56);
		strcpy(szContent + DEVICE_ID_LEN + 1 + 56, stDeviceInfo.ParaValues);
		int contentLen = DEVICE_ID_LEN + 1 + 56 + strlen(stDeviceInfo.ParaValues);
		outLen += contentLen;
		memcpy(outBuf + 28, szContent, contentLen);

		ret = SYS_STATUS_SUCCESS;
	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	return ret;
}
//设备添加
int Net_Dev_Add(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED; 
	//业务包指针
	PStrNetDevUpdate pRecMsg = (PStrNetDevUpdate)inBuf;

	memcpy(outBuf, inBuf, 28);

	char deviceType[DEVICE_TYPE_LEN_TEMP] = {0};
	char id[DEVICE_ID_LEN_TEMP] = {0};
	char CNAME[31] = {0};
	char Upper[DEVICE_ID_LEN_TEMP] = {0};
	char Upper_Channel[8] = {0};
	char CTYPE[3] = {0};
	char self_id[21] = {0};
	char private_attr[256] = {0};
	char szBrief[21] = {0};			//设备简称
	char szReserve2[129] = {0};		
	char szMemo[129] = {0};

	memcpy(deviceType, pRecMsg->szDevType, DEVICE_TYPE_LEN);
	memcpy(id, pRecMsg->szDevType, DEVICE_ID_LEN);
	memcpy(CNAME, pRecMsg->szCName, 30);
	memcpy(Upper_Channel, pRecMsg->szChannelId, 2);
	memcpy(Upper, pRecMsg->szReserve1, DEVICE_ID_LEN);
	memcpy(CTYPE, pRecMsg->szDevAppType, 2);
	memcpy(self_id, pRecMsg->szSelfId, 20);
	memcpy(szBrief, pRecMsg->szBrief, 20);
	szBrief[20] = '\0';
	memcpy(private_attr, pRecMsg->szPrivateAttr, 255);
	memcpy(szReserve2, pRecMsg->szReserve2, 128);
	memcpy(szMemo, pRecMsg->szMemo, 128);
	do 
	{		
		const char* sqlFormat = NULL;
		char sql[512] = {0};

		//检查设备条数是否超标lsd
		char queryResult[512] = {0};
		if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"device_detail", (char*)"count(*)", (char*)"", queryResult))
		{
			ret = SYS_STATUS_DEVICE_COUNT_MAX;
			break;
		}
		char* split_result = NULL;
		char* savePtr = NULL;
		split_result = strtok_r(queryResult, "#", &savePtr);
		if (atoi(split_result) + 1 > g_RecordCntCfg.Dev_Max)
		{
			ret = SYS_STATUS_DEVICE_COUNT_MAX;
			break;
		}

		//判断设备类型是否支持
		sqlFormat = "where id = '%s'";
		sprintf(sql, sqlFormat, deviceType);
		memset(queryResult, 0, 512);
		if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"device_info", (char*)"mode_list", sql, queryResult))
		{
			ret = SYS_STATUS_DEVICE_TYPE_NOT_SURPORT;
			break;
		}
		split_result = NULL;
		savePtr = NULL;
		char szModeList[512] = {0};
		split_result = strtok_r(queryResult, "#", &savePtr);
		if (NULL == split_result)
		{
			ret = SYS_STATUS_DEVICE_TYPE_NOT_SURPORT;
			break;
		}
		strcpy(szModeList, split_result);

		//加入到数据库Device_Detail表,并在device_roll,device_act添加相应的记录
		if (!g_SqlCtl.AppAddDeviceDetail(trim(id, strlen(id)), trim(CNAME, strlen(CNAME)), trim(Upper, strlen(Upper))
			, trim(Upper_Channel, strlen(Upper_Channel)), trim(CTYPE, strlen(CTYPE)), trim(self_id, strlen(self_id)), 
			'2', trim(private_attr, strlen(private_attr)), trim(szBrief, strlen(szBrief)), trim(szReserve2, strlen(szReserve2)), trim(szMemo, strlen(szMemo))))
		{
			break;
		}
		
		//插入到设备明细表
		DEVICE_DETAIL_INFO stDeviceDetail;
		memset((unsigned char*)&stDeviceDetail, 0, sizeof(DEVICE_DETAIL_INFO));

		memcpy(stDeviceDetail.Id, id, DEVICE_ID_LEN);
		memcpy(stDeviceDetail.szCName, CNAME, 30);
		stDeviceDetail.cStatus = 2;
		strcpy(stDeviceDetail.szSelfId, self_id);
		strcpy(stDeviceDetail.szBreaf, szBrief);
		strcpy(stDeviceDetail.szModList, szModeList);
		define_sscanf(private_attr, (char*)"login_pwd=", stDeviceDetail.szPwd);
		stDeviceDetail.iChannel = atoi(Upper_Channel);
		strcpy(stDeviceDetail.Upper, Upper);
		//初始化
		g_DevInfoContainer.InsertDeviceDetailNode(stDeviceDetail);//初始化设备表
		g_DevInfoContainer.InitDeviceModeNode(stDeviceDetail.Id, stDeviceDetail.szModList);//初始化设备模式表

		//插入到设备采集缓存表V10.0.0.18		
		const char* column = "select device_roll.id, "
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
			"device_detail.STATUS, "//V10.0.0.18
			"device_attr.card, "
			"device_detail.upper, "
			"device_attr.value_limit, "
			"device_info.private_attr "
			" from device_roll, device_detail, device_attr, device_info "
			" where device_roll.id = device_detail.id "
			" and substr(device_roll.id,1,%d) = device_attr.id"
			" and device_roll.sn = device_attr.sn"
			" and substr(device_roll.id,1,%d) = device_info.id"//V10.0.0.18
			" and device_roll.id = '%s'"
			" group by device_roll.id, device_roll.sn;";

		memset(sql, 0, sizeof(sql));
		sprintf(sql, column, DEVICE_TYPE_LEN, DEVICE_TYPE_LEN, trim(id, strlen(id)));
		if(!g_DevInfoContainer.InitDeviceRollInfo(sql))
			break;

		//添加到网络口
		if (INTERFACE_NET_INPUT <= atoi(trim(Upper_Channel, strlen(Upper_Channel))))
		{
			g_NetContainer.addNode(id);
		}

		//插入到设备动作缓存表V10.0.0.18
		sqlFormat = "select device_act.id, "
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
							"and device_act.id = '%s' "
							"group by device_act.id, device_act.sn ;";

		memset(sql, 0, sizeof(sql));
		sprintf(sql, sqlFormat, DEVICE_TYPE_LEN, DEVICE_TYPE_LEN, trim(id, strlen(id)));
		if(!g_DevInfoContainer.InitDeviceActionInfo(sql))
			break;

		//插入到Analyser分析表
		DBG(("Now try to init actHash--------------->\n"));
		sqlFormat = "select device_roll.id,"
			"device_roll.sn,"
			"device_detail.ctype,"
			"device_detail.status,"
			"device_roll.standard,"
			"device_attr.s_define "
			" from device_roll, "
			" device_detail,"
			" device_attr"
			" where device_roll.id = device_detail.id "
			" and device_roll.sn = device_attr.sn"
			" and substr(device_detail.id,1,%d) = device_attr.id "
			" and device_roll.id = '%s'"
			" group by device_roll.id, device_roll.sn;";

		memset(sql, 0, sizeof(sql));
		sprintf(sql, sqlFormat, DEVICE_TYPE_LEN, trim(id, strlen(id)));

		if (!g_Analyser.InitActHashData(sql))
		{
			DBG(("g_Analyser.InitActHashData Failed ....\n"));
			break;
		}

		ret = SYS_STATUS_SUCCESS;
	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	outLen = 28;
	return ret;
}
//设备更新
int Net_Dev_Update(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED;
	//业务包指针
	PStrNetDevUpdate pRecMsg = (PStrNetDevUpdate)inBuf;

	memcpy(outBuf, inBuf, 28);

	char szDevId[DEVICE_ID_LEN_TEMP] = {0};			//设备编号
	char szParentDeviceId[DEVICE_ID_LEN_TEMP] = {0}; //上级设备
	char szDevAppType[3] = {0};		//设备应用类型
	char szChannelId[3] = {0};		//通道编号
	char szSelfId[21] = {0};		//子通道号
//	char cStatus = '0';				//‘0’停用，‘1’启用
	char szCName[31] = {0};			//设备名称
	char szBrief[21] = {0};			//设备简称
	char szPrivateAttr[256] = {0};	//设备私有参数
	char szReserve2[129] = {0};		
	char szMemo[129] = {0};

	memcpy(szDevId, pRecMsg->szDevType, DEVICE_ID_LEN);
	szDevId[DEVICE_ID_LEN] = '\0';
	memcpy(szParentDeviceId, pRecMsg->szReserve1, DEVICE_ID_LEN);
	szParentDeviceId[DEVICE_ID_LEN] = '\0';
	memcpy(szDevAppType, pRecMsg->szDevAppType, 2);
	szDevAppType[2] = '\0';
	memcpy(szChannelId, pRecMsg->szChannelId, 2);
	szChannelId[2] = '\0';
	memcpy(szSelfId, pRecMsg->szSelfId, 20);
	szSelfId[20] = '\0';
//	cStatus = pRecMsg->cStatus;
	memcpy(szCName, pRecMsg->szCName, 30);
	szCName[30] = '\0';
	memcpy(szBrief, pRecMsg->szBrief, 20);
	szBrief[20] = '\0';
	memcpy(szPrivateAttr, pRecMsg->szPrivateAttr, 255);
	szPrivateAttr[255] = '\0';

	memcpy(szReserve2, pRecMsg->szReserve2, 128);
	memcpy(szMemo, pRecMsg->szMemo, 128);	

	do 
	{	
		//检查设备是否存在
		const char* sqlFormat = NULL;
		char sql[512] = {0};

		sqlFormat = "where id = '%s'";
		sprintf(sql, sqlFormat, szDevId);

		char queryResult[128] = {0};
		if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"device_detail", (char*)"count(*)", sql, queryResult))
		{
			ret = SYS_STATUS_DEVICE_TYPE_NOT_SURPORT;
			break;
		}

		char* split_result = NULL;
		char* savePtr = NULL;
		split_result = strtok_r(queryResult, "#", &savePtr);
		if (NULL == split_result)
		{
			ret = SYS_STATUS_DEVICE_TYPE_NOT_SURPORT;
			break;
		}

		if (1 != atoi(split_result))
		{
			ret = SYS_STATUS_DEVICE_NOT_EXIST;
			break;
		}

		//更新到数据库Device_Detail表
		sqlFormat = "update device_detail set cname = '%s', upper = '%s', upper_channel = '%s', ctype = '%s', self_id = '%s', private_attr = '%s', brief = '%s', preset='%s', memo='%s' where id = '%s';";

		sprintf(sql, sqlFormat, trim(szCName, strlen(szCName)), trim(szParentDeviceId, strlen(szParentDeviceId)), trim(szChannelId, strlen(szChannelId))
			, trim(szDevAppType, strlen(szDevAppType)), trim(szSelfId, strlen(szSelfId)), trim(szPrivateAttr, strlen(szPrivateAttr))
			,trim(szBrief, strlen(szBrief)), trim(szReserve2, strlen(szReserve2)), trim(szMemo, strlen(szMemo)), trim(szDevId, strlen(szDevId)));
		if(!g_SqlCtl.ExecSql(sql, NULL, NULL, false, 0))//lhy ，如果不成功怎么处理
		{
			break;
		}

		//更新到设备明细表
		DEVICE_DETAIL_INFO stDeviceDetail;
		memset((unsigned char*)&stDeviceDetail, 0, sizeof(DEVICE_DETAIL_INFO));
		memcpy(stDeviceDetail.Id, szDevId, DEVICE_ID_LEN);
		memcpy(stDeviceDetail.szCName, szCName, 30);
//		stDeviceDetail.cStatus = atoi(&cStatus);
		strcpy(stDeviceDetail.szSelfId, szSelfId);
		strcpy(stDeviceDetail.szBreaf, szBrief);
		define_sscanf(szPrivateAttr, (char*)"login_pwd=", stDeviceDetail.szPwd);
		stDeviceDetail.iChannel = atoi(szChannelId);
		strncpy(stDeviceDetail.Upper, szParentDeviceId, 10);
		g_DevInfoContainer.UpdateDeviceDetailNode(stDeviceDetail);		//lsd 11.4

		NET_DEVICE_INFO_UPDATE stDevInfoUpdate;
		memset((unsigned char*)&stDevInfoUpdate, 0, sizeof(NET_DEVICE_INFO_UPDATE));
		strcpy(stDevInfoUpdate.Id, trim(szDevId, strlen(szDevId)));
		strcpy(stDevInfoUpdate.Upper_Channel, trim(szChannelId, strlen(szChannelId)));
		strcpy(stDevInfoUpdate.Self_Id, trim(szSelfId, strlen(szSelfId)));
//		stDevInfoUpdate.Status = cStatus;
		strcpy(stDevInfoUpdate.PrivateAttr, trim(szPrivateAttr, strlen(szPrivateAttr)));

		//更新设备属性表
		g_DevInfoContainer.QueryHashUpdateByDevice(stDevInfoUpdate);

		//更新动作表
		g_DevInfoContainer.ActionHashUpdateByNet(stDevInfoUpdate);

		ret = SYS_STATUS_SUCCESS;

	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	outLen = 28;

	return ret;
}

//设备删除
int Net_Dev_Del(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED;
	//业务包指针
	PStrNetDevDel pRecMsg = (PStrNetDevDel)inBuf;

	memcpy(outBuf, inBuf, 28);

	char id[DEVICE_ID_LEN_TEMP] = {0};

	memcpy(id, pRecMsg->szDevType, DEVICE_ID_LEN);
	do 
	{
		//定时任务中是否有对应设备的配置，如果有则返回失败lhy
		const char* sqlFormat = "where id = '%s'";
		char sql[512] = {0};

		sprintf(sql, sqlFormat, id);

		char queryResult[128] = {0};
		if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"TASK_INFO", (char*)"count(*)", sql, queryResult))
		{
			break;
		}

		char* split_result = NULL;
		char* savePtr = NULL;
		split_result = strtok_r(queryResult, "#", &savePtr);
		if (NULL == split_result)
		{
			break;
		}

		if (1 <= atoi(split_result))
		{
			ret = SYS_STATUS_TASK_INFO_NOT_EMPTY;
			break;
		}

		//判断联动表是否有对应设备配置
		sqlFormat = "where id = '%s' or condition like '%%P%s%%' or d_id = '%s'";
		memset(sql, 0, sizeof(sql));
		sprintf(sql, sqlFormat, id, id, id);
		memset(queryResult, 0, sizeof(queryResult));
		if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"DEVICE_AGENT", (char*)"count(*)", sql, queryResult))
		{
			break;
		}

		split_result = NULL;
		savePtr = NULL;
		split_result = strtok_r(queryResult, "#", &savePtr);
		if (NULL == split_result)
		{
			break;
		}

		if (1 <= atoi(split_result))
		{
			ret = SYS_STATUS_AGENT_INFO_NOT_EMPTY;
			break;
		}

		//判断联动表是否为其他设备的私有属性
		sqlFormat = "where V_ID = '%s'";
		memset(sql, 0, sizeof(sql));
		sprintf(sql, sqlFormat, id);
		memset(queryResult, 0, sizeof(queryResult));
		if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"DEVICE_ROLL", (char*)"count(*)", sql, queryResult))
		{
			break;
		}

		split_result = NULL;
		savePtr = NULL;
		split_result = strtok_r(queryResult, "#", &savePtr);
		if (NULL == split_result)
		{
			break;
		}

		if (1 <= atoi(split_result))
		{
			ret = SYS_STATUS_ATTR_USED_BY_OTHER;
			break;
		}

		//判断防区表是否有该设备
		sqlFormat = "where point like '%%%s%%' and length(id) = 8";
		memset(sql, 0, sizeof(sql));
		sprintf(sql, sqlFormat, id);
		memset(queryResult, 0, sizeof(queryResult));
		if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"ROLE", (char*)"count(*)", sql, queryResult))
		{
			break;
		}

		split_result = NULL;
		savePtr = NULL;
		split_result = strtok_r(queryResult, "#", &savePtr);
		if (NULL == split_result)
		{
			break;
		}

		if (1 <= atoi(split_result))
		{
			ret = SYS_STATUS_ROLE_NOT_EMPTY;
			break;
		}


		//删除device_roll表中相关记录
		const char* sqlFormatRoll = "delete from device_roll where id like '%s%c';";
		memset(sql, 0, sizeof(sql));
		sprintf(sql, sqlFormatRoll, trim(id, strlen(id)), '%');
		if(!g_SqlCtl.ExecSql(sql, NULL, NULL, false, 0))
		{
			break;
		}

		//删除device_act表中相关记录V10.0.0.18
		const char* sqlFormatAct = "delete from device_act where id like '%s%c';";
		memset(sql, 0, sizeof(sql));
		sprintf(sql, sqlFormatAct, trim(id, strlen(id)), '%');
		if(!g_SqlCtl.ExecSql(sql, NULL, NULL, false, 0))
		{
			break;
		}

		const char* sqlFormatDetail = "delete from device_detail where id = '%s';";
		memset(sql, 0, sizeof(sql));
		sprintf(sql, sqlFormatDetail, trim(id, strlen(id)));
		//删除device_detail表中相关记录
		if(!g_SqlCtl.ExecSql(sql, NULL, NULL, false, 0))
		{
			break;
		}

		//从设备明细表删除
		DEVICE_DETAIL_INFO stDeviceDetail;
		memset((unsigned char*)&stDeviceDetail, 0, sizeof(DEVICE_DETAIL_INFO));
		memcpy(stDeviceDetail.Id, id, DEVICE_ID_LEN);

		DBG(("Net_Dev_Del DeleteDeviceDetailNode\n"));
		g_DevInfoContainer.DeleteDeviceDetailNode(stDeviceDetail);
		
		//从设备模式表中删除
		g_DevInfoContainer.ModeHashDeleteByDevice(id);
		

		NET_DEVICE_INFO_UPDATE stDevInfoUpdate;
		memset((unsigned char*)&stDevInfoUpdate, 0, sizeof(NET_DEVICE_INFO_UPDATE));
		strcpy(stDevInfoUpdate.Id, trim(id, strlen(id)));
		
		//删除设备轮巡表
		DBG(("Net_Dev_Del QueryHashDeleteByDevice111\n"));
		g_DevInfoContainer.QueryHashDeleteByDevice(stDevInfoUpdate);
		DBG(("Net_Dev_Del QueryHashDeleteByDevice\n"));

		//删除设备动作
		g_DevInfoContainer.ActionHashDeleteByNet(stDevInfoUpdate);
		DBG(("Net_Dev_Del ActionHashDeleteByNet\n"));

		//更新Analyser联动表
		g_Analyser.AnalyseHashDeleteByNet(trim(id, strlen(id)));
		DBG(("Net_Dev_Del AnalyseHashDeleteByNet\n"));

		g_NetContainer.deleteNodeByDevice(trim(id, strlen(id)));
		ret = SYS_STATUS_SUCCESS;
	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	outLen = 28;
	return ret;
}

//设备采集属性修改
int Net_Dev_Attr_Update(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED;
	//业务包指针
	PStrNetDevAttrUpdate pRecMsg = (PStrNetDevAttrUpdate)inBuf;

	char szDeviceId[DEVICE_ID_LEN_TEMP] = {0};
	char szAttrId[DEVICE_SN_LEN_TEMP] = {0};
	char szSecureTime[21] = {0};
	char szStandardValue[61] = {0};
	char szPrivateAttr[256] = {0};
	char szV_Id[DEVICE_ID_LEN_TEMP] = {0};					//虚拟设备编号
	char szV_AttrId[DEVICE_SN_LEN_TEMP] = {0};				//虚拟设备属性编号
	char cStaShow = '0';
	char szS_define[129] = {0};
	char szAttr_Name[21] = {0};
	char cStatus = '0';
	char szValue_Limit[65] = {0};

	memcpy(outBuf, inBuf, 28);

	memcpy(szDeviceId, pRecMsg->szDevType, DEVICE_ID_LEN);
	memcpy(szAttrId, pRecMsg->szAttrId, DEVICE_SN_LEN);
	memcpy(szSecureTime, pRecMsg->szSecureTime, 20);
	memcpy(szStandardValue, pRecMsg->szStandardValue, 60);
	memcpy(szPrivateAttr, pRecMsg->szPrivateAttr, 255);
	memcpy(szV_Id, pRecMsg->szV_DevType, DEVICE_ID_LEN);
	memcpy(szV_AttrId, pRecMsg->szV_AttrId, DEVICE_SN_LEN);
	cStaShow = pRecMsg->cIsShow;

	memcpy(szS_define, pRecMsg->szS_define, 128);
	memcpy(szAttr_Name, pRecMsg->szAttr_Name, 20);
	cStatus = pRecMsg->cStatus;
	memcpy(szValue_Limit, pRecMsg->szValueLimit, 64);

	do 
	{		
		DBG(("Net_Dev_Attr_Update11111111\n"));
		//更新到数据库Device_Roll表
		const char* sqlFormat = "update device_roll "
							"set private_attr = '%s', "
							"standard = '%s', "
							"defence = '%s', "
							"v_id = '%s', "
							"v_sn = '%s', "
							"sta_show = '%c', "
							"attr_name='%s', "
							"s_define='%s', "
							"Status='%c', "
							"Value_Limit='%s' "
							"where id = '%s' and sn = '%s';";
		char sql[512] = {0};
		sprintf(sql, sqlFormat, 
			trim(szPrivateAttr, strlen(szPrivateAttr)), 
			trim(szStandardValue, strlen(szStandardValue)), 
			trim(szSecureTime, strlen(szSecureTime)), 
			trim(szV_Id, strlen(szV_Id)), 
			trim(szV_AttrId, strlen(szV_AttrId)), 
			cStaShow, 
			trim(szAttr_Name, strlen(szAttr_Name)), 
			trim(szS_define, strlen(szS_define)), 
			cStatus,
			trim(szValue_Limit, strlen(szValue_Limit)), 
			trim(szDeviceId, strlen(szDeviceId)), 
			trim(szAttrId, strlen(szAttrId)));
		if(!g_SqlCtl.ExecSql(sql, NULL, NULL, false, 0))
		{
			break;
		}

		DBG(("Net_Dev_Attr_Update 2222222\n"));
		NET_DEVICE_ATTR_INFO_UPDATE stNetDevAttrUpdate;
		memset((unsigned char*)&stNetDevAttrUpdate, 0, sizeof(NET_DEVICE_ATTR_INFO_UPDATE));
		strcpy(stNetDevAttrUpdate.Id, trim(szDeviceId, strlen(szDeviceId)));
		strcpy(stNetDevAttrUpdate.AttrId, trim(szAttrId, strlen(szAttrId)));
		strcpy(stNetDevAttrUpdate.SecureTime, trim(szSecureTime, strlen(szSecureTime)));
		strcpy(stNetDevAttrUpdate.StandardValue, trim(szStandardValue, strlen(szStandardValue)));
		strcpy(stNetDevAttrUpdate.PrivateAttr, trim(szPrivateAttr, strlen(szPrivateAttr)));
		strcpy(stNetDevAttrUpdate.szV_Id, trim(szV_Id, strlen(szV_Id)));
		strcpy(stNetDevAttrUpdate.szV_AttrId, trim(szV_AttrId, strlen(szV_AttrId)));
		stNetDevAttrUpdate.cStaShow = cStaShow;
	
		strcpy(stNetDevAttrUpdate.szS_define, trim(szS_define, strlen(szS_define)));
		strcpy(stNetDevAttrUpdate.szAttr_Name, trim(szAttr_Name, strlen(szAttr_Name)));
		stNetDevAttrUpdate.cStatus = cStatus;
		strcpy(stNetDevAttrUpdate.szValueLimit, trim(szValue_Limit, strlen(szValue_Limit)));

		//更新到采集属性表
		g_DevInfoContainer.QueryHashUpdateByDeviceAttr(stNetDevAttrUpdate);

		DBG(("Net_Dev_Attr_Update 33333333\n"));
		ret = SYS_STATUS_SUCCESS;
	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	outLen = 28;
	return ret;
}

//设备动作属性修改
int Net_Dev_Action_Update(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED;
	//业务包指针
	PStrNetDevActionUpdate pRecMsg = (PStrNetDevActionUpdate)inBuf;

	char szDeviceId[DEVICE_ID_LEN_TEMP] = {0};
	char szActionId[DEVICE_ACTION_SN_LEN_TEMP] = {0};				//设备采集属性编号
	char szActionName[21] = {0};			//布防时间
	char cStatus  = '0';
	char szPrivateAttr[256] = {0};			//私有属性
	char szVDevid[DEVICE_ID_LEN_TEMP] = {0};					//对应实体设备的编号
	char szVActionId[DEVICE_ACTION_SN_LEN_TEMP] = {0};				//对应实体设备的动作编号

	memcpy(outBuf, inBuf, 28);

	memcpy(szDeviceId, pRecMsg->szDevType, DEVICE_ID_LEN);
	memcpy(szActionId, pRecMsg->szActionId, DEVICE_ACTION_SN_LEN);
	memcpy(szActionName, pRecMsg->szActionName, 20);	
	cStatus = pRecMsg->cStatus;
	memcpy(szPrivateAttr, pRecMsg->szPrivateAttr, 255);
	memcpy(szVDevid, pRecMsg->szVDevType, DEVICE_ID_LEN);
	memcpy(szVActionId, pRecMsg->szVActionId, DEVICE_ACTION_SN_LEN);
	do 
	{		
		DBG(("Net_Dev_Action_Update 111111111\n"));
		char queryResult[128] = {0};
		//检查设备总联动条数是否超标lsd
		if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"device_act", (char*)"count(*)", (char*)"where status='1'", queryResult))
		{
			ret = SYS_STATUS_DEVICE_ACTION_MAX;
			break;
		}
		char* split_result = NULL;
		char* savePtr = NULL;
		split_result = strtok_r(queryResult, "#", &savePtr);
		if (atoi(split_result + 1) > g_RecordCntCfg.Action_Max)
		{
			ret = SYS_STATUS_DEVICE_ACTION_MAX;
			break;
		}

		//更新到数据库Device_Act表
		const char* sqlFormat = "update device_act "
			" set cmdname='%s', "
			" Status='%c', "
			" private_attr='%s',"
			" v_id = '%s',"
			" v_sn = '%s'"
			" where id = '%s' and sn = '%s';";
		char sql[512] = {0};
		sprintf(sql, sqlFormat, 
			trim(szActionName, strlen(szActionName)), 
			cStatus,
			trim(szPrivateAttr, strlen(szPrivateAttr)),
			trim(szVDevid, strlen(szVDevid)),
			trim(szVActionId, strlen(szVActionId)),
			trim(szDeviceId, strlen(szDeviceId)), 
			trim(szActionId, strlen(szActionId)));
		if(!g_SqlCtl.ExecSql(sql, NULL, NULL, false, 0))
		{
			break;
		}
		
		//更新到动作属性表
		g_DevInfoContainer.UpdateDeviceActionStatus(szDeviceId, 
													szActionId, 
													cStatus, 
													trim(szActionName, strlen(szActionName)), 
													trim(szVDevid, strlen(szVDevid)),
													trim(szVActionId, strlen(szVActionId))
													);

		DBG(("Net_Dev_Attr_Update 33333333\n"));
		ret = SYS_STATUS_SUCCESS;
	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	outLen = 28;
	return ret;
}
//设备联动配置添加
int Net_Dev_Agent_Add(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED;
	//业务包指针
	PStrNetDevAgentUpdate pRecMsg = (PStrNetDevAgentUpdate)inBuf;

	memcpy(outBuf, inBuf, 28);

	AgentInfo stAgentInfo;
	memset((unsigned char*)&stAgentInfo, 0, sizeof(AgentInfo));	

	char temp[11] = {0};
	memcpy(temp, pRecMsg->szSeq, 10);
	stAgentInfo.iSeq = atoi(temp);

	char szDevId[DEVICE_ID_LEN_TEMP] = {0};
	memcpy(szDevId, pRecMsg->szDevType, DEVICE_ID_LEN);
	strcpy(stAgentInfo.szDevId, trim(szDevId, strlen(szDevId)));

	char szAttrId[DEVICE_SN_LEN_TEMP] = {0};
	memcpy(szAttrId, pRecMsg->szAttrId, DEVICE_SN_LEN);
	strcpy(stAgentInfo.szAttrId, trim(szAttrId, strlen(szAttrId)));

	char szCondition[129] = {0};
	memcpy(szCondition, pRecMsg->szCondition, 128);
	strcpy(stAgentInfo.szCondition, trim(szCondition, strlen(szCondition)));

	memset(temp, 0, sizeof(temp));
	memcpy(temp, pRecMsg->szInterval, 4);
	stAgentInfo.iInterval = atoi(trim(temp, strlen(temp)));

	memset(temp, 0, sizeof(temp));
	memcpy(temp, pRecMsg->szTimes, 4);
	stAgentInfo.iTimes = atoi(trim(temp, strlen(temp)));

	char szValibtime[31] = {0};
	memcpy(szValibtime, pRecMsg->szValibtime, 30);
	strcpy(stAgentInfo.szValibtime, trim(szValibtime, strlen(szValibtime)));

	char szActDevId[DEVICE_ID_LEN_TEMP] = {0};
	memcpy(szActDevId, pRecMsg->szActDevType, DEVICE_ID_LEN);
	strcpy(stAgentInfo.szActDevId, trim(szActDevId, strlen(szActDevId)));

	char szActActionId[DEVICE_ACTION_SN_LEN_TEMP] = {0};
	memcpy(szActActionId, pRecMsg->szActActionId, DEVICE_ACTION_SN_LEN);
	strcpy(stAgentInfo.szActActionId, trim(szActActionId, strlen(szActActionId)));

	char szActValue[257] = {0};
	memcpy(szActValue, pRecMsg->szActValue, 256);
	strcpy(stAgentInfo.szActValue, trim(szActValue, strlen(szActValue)));

	stAgentInfo.cType = pRecMsg->cType;

	do 
	{		
		char queryResult[128] = {0};
		//检查设备总联动条数是否超标lsd
		if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"device_agent", (char*)"count(*)", (char*)"", queryResult))
		{
			ret = SYS_STATUS_DEVICE_AGENT_MAX;
			break;
		}
		char* split_result = NULL;
		char* savePtr = NULL;
		split_result = strtok_r(queryResult, "#", &savePtr);
		if (atoi(split_result + 1) > g_RecordCntCfg.Agent_Max)
		{
			ret = SYS_STATUS_DEVICE_AGENT_MAX;
			break;
		}
		//检查该属性单条联动条数是否超标lsd
		char sql[512] = {0};
		const char* sqlFormat = " where id = '%s' and sn = '%s' ";
		memset(queryResult, 0, 128);
		sprintf(sql, sqlFormat, stAgentInfo.szDevId, stAgentInfo.szAttrId);
		if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"device_agent", (char*)"count(*)", sql, queryResult))
		{
			ret = SYS_STATUS_DEVICE_SINGLE_AGENT_MAX;
			break;
		}
		split_result = NULL;
		savePtr = NULL;
		split_result = strtok_r(queryResult, "#", &savePtr);
		if (atoi(split_result + 1) > g_RecordCntCfg.Single_Agent_Max)
		{
			ret = SYS_STATUS_DEVICE_SINGLE_AGENT_MAX;
			break;
		}

		//检查联动SEQ是否存在		
		split_result = NULL;
		savePtr = NULL;
		sqlFormat = "where seq = %d";
		memset(sql, 0, 512);
		memset(queryResult, 0, 128);
		sprintf(sql, sqlFormat, stAgentInfo.iSeq);		
		if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"device_agent", (char*)"count(*)", sql, queryResult))
		{
			break;
		}

		split_result = strtok_r(queryResult, "#", &savePtr);
		if (NULL == split_result)
		{
			break;
		}

		if (0 < atoi(split_result))
		{
			break;
		}

		if(SYS_STATUS_SUCCESS !=( ret= g_Analyser.AnalyseHashAgentCheck(stAgentInfo)))
		{
			DBG(("AnalyseHashAgentCheck Failed........\n"));
			break;
		}
		
		//添加到数据库Device_agent表
		sqlFormat = "insert into device_agent(SEQ, ID, SN, Condition, Valibtime, Interval, Times, D_ID, D_CMD, OBJECT, CTYPE)"
					" values('%d', '%s', '%s', '%s', '%s', '%d', '%d', '%s', '%s', '%s', '%c');";

		sprintf(sql, sqlFormat, 
			stAgentInfo.iSeq, 
			stAgentInfo.szDevId ,
			stAgentInfo.szAttrId,
			stAgentInfo.szCondition,
			stAgentInfo.szValibtime,
			stAgentInfo.iInterval,
			stAgentInfo.iTimes,
			stAgentInfo.szActDevId,
			stAgentInfo.szActActionId, 
			stAgentInfo.szActValue,
			stAgentInfo.cType);

		if(!g_SqlCtl.ExecSql(sql, NULL, NULL, false, 0))
		{
			ret = SYS_STATUS_SYS_BUSY;
			break;
		}

		//添加到Analyser联动表
		ret = g_Analyser.AnalyseHashAgentAdd(stAgentInfo);
		if(SYS_STATUS_SUCCESS != ret)
		{
			if(SYS_STATUS_DEVICE_ANALYSE_ACTION_FULL == ret)
			{
				//数据回滚
				memset(sql, 0, sizeof(sql));
				sqlFormat = "delete from device_agent where seq = '%d';";

				sprintf(sql, sqlFormat, stAgentInfo.iSeq);
				g_SqlCtl.ExecSql(sql, NULL, NULL, true, 0);
			}
			break;
		}

	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	outLen = 28;
	return ret;
}

//设备联动配置修改
int Net_Dev_Agent_Update(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED;
	//业务包指针
	PStrNetDevAgentUpdate pRecMsg = (PStrNetDevAgentUpdate)inBuf;

	memcpy(outBuf, inBuf, 28);

	AgentInfo stAgentInfo;
	memset((unsigned char*)&stAgentInfo, 0, sizeof(AgentInfo));

	char temp[11] = {0};
	memcpy(temp, pRecMsg->szSeq, 10);
	stAgentInfo.iSeq = atoi(temp);

	char szDevId[DEVICE_ID_LEN_TEMP] = {0};
	memcpy(szDevId, pRecMsg->szDevType, DEVICE_ID_LEN);
	strcpy(stAgentInfo.szDevId, trim(szDevId, strlen(szDevId)));

	DBG(("szDevId[%s] stDevid[%s]\n", szDevId, stAgentInfo.szDevId));

	char szAttrId[DEVICE_SN_LEN_TEMP] = {0};
	memcpy(szAttrId, pRecMsg->szAttrId, DEVICE_SN_LEN);
	strcpy(stAgentInfo.szAttrId, trim(szAttrId, strlen(szAttrId)));

	char szCondition[129] = {0};
	memcpy(szCondition, pRecMsg->szCondition, 128);
	strcpy(stAgentInfo.szCondition, trim(szCondition, strlen(szCondition)));

	memset(temp, 0, sizeof(temp));
	memcpy(temp, pRecMsg->szInterval, 4);
	stAgentInfo.iInterval = atoi(trim(temp, strlen(temp)));

	memset(temp, 0, sizeof(temp));
	memcpy(temp, pRecMsg->szTimes, 4);
	stAgentInfo.iTimes = atoi(trim(temp, strlen(temp)));

	char szValibtime[31] = {0};
	memcpy(szValibtime, pRecMsg->szValibtime, 30);
	strcpy(stAgentInfo.szValibtime, trim(szValibtime, strlen(szValibtime)));

	char szActDevId[DEVICE_ID_LEN_TEMP] = {0};
	memcpy(szActDevId, pRecMsg->szActDevType, DEVICE_ID_LEN);
	strcpy(stAgentInfo.szActDevId, trim(szActDevId, strlen(szActDevId)));

	char szActActionId[DEVICE_ACTION_SN_LEN_TEMP] = {0};
	memcpy(szActActionId, pRecMsg->szActActionId, DEVICE_ACTION_SN_LEN);
	strcpy(stAgentInfo.szActActionId, trim(szActActionId, strlen(szActActionId)));

	char szActValue[257] = {0};
	memcpy(szActValue, pRecMsg->szActValue, 256);
	strcpy(stAgentInfo.szActValue, trim(szActValue, strlen(szActValue)));

	stAgentInfo.cType = pRecMsg->cType;

	do 
	{		
		//检查设备是否存在
		const char* sqlFormat = "where seq = %d";
		char sql[512] = {0};
		char* split_result = NULL;
		char* savePtr = NULL;

		sprintf(sql, sqlFormat, stAgentInfo.iSeq);

		char queryResult[128] = {0};
		if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"device_agent", (char*)"count(*)", sql, queryResult))
		{
			break;
		}

		split_result = strtok_r(queryResult, "#", &savePtr);
		if (NULL == split_result || 1 != atoi(split_result))
		{
			break;
		}

		//添加到数据库Device_agent表
		sqlFormat = "update device_agent set ID = '%s', SN = '%s', Condition = '%s', Valibtime = '%s', Interval = '%d', Times = '%d', D_ID = '%s', D_CMD = '%s', OBJECT = '%s', CTYPE = '%c' where seq = %d;";
		sprintf(sql, sqlFormat, 
				trim(stAgentInfo.szDevId, strlen(stAgentInfo.szDevId)), 
				trim(stAgentInfo.szAttrId, strlen(stAgentInfo.szAttrId)),
				trim(stAgentInfo.szCondition, strlen(stAgentInfo.szCondition)),
				trim(stAgentInfo.szValibtime, strlen(stAgentInfo.szValibtime)),
				stAgentInfo.iInterval,
				stAgentInfo.iTimes,
				trim(stAgentInfo.szActDevId, strlen(stAgentInfo.szActDevId)), 
				trim(stAgentInfo.szActActionId, strlen(stAgentInfo.szActActionId)), 
				trim(stAgentInfo.szActValue, strlen(stAgentInfo.szActValue)), 
				stAgentInfo.cType,
				stAgentInfo.iSeq);

		if(!g_SqlCtl.ExecSql(sql, NULL, NULL, false, 0))
		{
			break;
		}

		//更新到Analyser联动表
		ret = g_Analyser.AnalyseHashAgentUpdate(stAgentInfo);
	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	outLen = 28;
	return ret;
}

//设备联动配置删除
int Net_Dev_Agent_Del(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED;
	//业务包指针
	PStrNetDevAgentUpdate pRecMsg = (PStrNetDevAgentUpdate)inBuf;
	memcpy(outBuf, inBuf, 28);

	AgentInfo stAgentInfo;
	memset((unsigned char*)&stAgentInfo, 0, sizeof(AgentInfo));

	char temp[11] = {0};
	memcpy(temp, pRecMsg->szSeq, 10);
	stAgentInfo.iSeq = atoi(temp);
	memcpy(stAgentInfo.szDevId, pRecMsg->szDevType, DEVICE_TYPE_LEN);
	memcpy(stAgentInfo.szDevId + DEVICE_TYPE_LEN, pRecMsg->szId, DEVICE_INDEX_LEN);
	memcpy(stAgentInfo.szAttrId, pRecMsg->szAttrId, DEVICE_SN_LEN);

	do 
	{		
		//删除数据库Device_Agent表中记录
		const char* sqlFormat = "delete from device_agent where seq = %d;";
		char sql[512] = {0};
		sprintf(sql, sqlFormat, stAgentInfo.iSeq);
		if(!g_SqlCtl.ExecSql(sql, NULL, NULL, false, 0))
		{
			break;
		}

		//从Analyser联动表删除
		g_Analyser.AnalyseHashAgentDelete(stAgentInfo);

		ret = SYS_STATUS_SUCCESS;
	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	outLen = 28;
	return ret;
}

//2008 定时任务添加
int Net_Time_Task_Add(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED;
	//业务包指针
	PStrNetTimeTaskUpdate pRecMsg = (PStrNetTimeTaskUpdate)inBuf;

	memcpy(outBuf, inBuf, 28);

	ONTIME_ACT_HASH hash_node;
	char seq[11] = {0};
	char id[DEVICE_ID_LEN_TEMP] = {0};
	char sn[DEVICE_ACTION_SN_LEN_TEMP] = {0};
	char ctime[13] = {0};
	char cdata[257] = {0};
	char memo [129] = {0};

	memcpy(seq, pRecMsg->szSeq, sizeof(pRecMsg->szSeq));
	memcpy(id, pRecMsg->szId, sizeof(pRecMsg->szId));
	memcpy(sn, pRecMsg->szSn, sizeof(pRecMsg->szSn));
	memcpy(ctime, pRecMsg->szTime, sizeof(pRecMsg->szTime));
	memcpy(cdata, pRecMsg->szValue, sizeof(pRecMsg->szValue));
	memcpy(memo, pRecMsg->szMemo, sizeof(pRecMsg->szMemo));

	memset((BYTE*)&hash_node, 0, sizeof(hash_node));
	hash_node.Seq = atoi(trim(seq, strlen(seq)));
	memcpy(hash_node.id, pRecMsg->szId, DEVICE_ID_LEN);
	memcpy(hash_node.id+DEVICE_ID_LEN,pRecMsg->szSn, DEVICE_ACTION_SN_LEN);
	memcpy(hash_node.szTime, pRecMsg->szTime, 12);
	hash_node.actTime = g_COnTimeCtrl.getNextActTime(hash_node.szTime);
	strcpy(hash_node.actValue, trim(cdata, strlen(cdata)));
	DBG(("Net_Time_Task_Add actTime[%ld]\n", hash_node.actTime));
	do
	{		
		char queryResult[128] = {0};
		//检查定时任务总条数是否超标lsd
		if(0 != g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"task_info", (char*)"count(*)", (char*)"", queryResult))
		{
			ret = SYS_STATUS_TASK_TIME_MAX;
			break;
		}
		char* split_result = NULL;
		char* savePtr = NULL;
		split_result = strtok_r(queryResult, "#", &savePtr);
		if (atoi(split_result) + 1 > g_RecordCntCfg.TimeCtrl_Max)
		{
			ret = SYS_STATUS_TASK_TIME_MAX;
			break;
		}

		//添加数据到Task_info表
		const char* sqlFormat = "insert into task_info(seq,id,sn,ctime,cdata,memo) values('%s', '%s', '%s', '%s', '%s', '%s');";
		char sql[512] = {0};
		sprintf(sql, sqlFormat, trim(seq, strlen(seq)), trim(id, strlen(id)), trim(sn, strlen(sn)), trim(ctime, strlen(ctime)), trim(cdata, strlen(cdata)), rtrim(memo, strlen(memo)));
		if(!g_SqlCtl.ExecSql(sql, NULL, NULL, false, 0))
		{
			break;
		}

		if(0 >= hash_node.actTime)
		{
			ret = SYS_STATUS_SUCCESS;
			break;
		}
		//添加数据到定时任务表
		g_COnTimeCtrl.addOnTimeNode(hash_node);
		ret = SYS_STATUS_SUCCESS;
	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	outLen = 28;
	return ret;
}

//2009 定时任务修改
int Net_Time_Task_Update(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED;
	//业务包指针
	PStrNetTimeTaskUpdate pRecMsg = (PStrNetTimeTaskUpdate)inBuf;

	memcpy(outBuf, inBuf, 28);

	ONTIME_ACT_HASH hash_node;
	char seq[11] = {0};
	char id[DEVICE_ID_LEN_TEMP] = {0};
	char sn[DEVICE_ACTION_SN_LEN_TEMP] = {0};
	char ctime[13] = {0};
	char cdata[257] = {0};
	char memo [129] = {0};

	memcpy(seq, pRecMsg->szSeq, sizeof(pRecMsg->szSeq));
	memcpy(id, pRecMsg->szId, sizeof(pRecMsg->szId));
	memcpy(sn, pRecMsg->szSn, sizeof(pRecMsg->szSn));
	memcpy(ctime, pRecMsg->szTime, sizeof(pRecMsg->szTime));
	memcpy(cdata, pRecMsg->szValue, sizeof(pRecMsg->szValue));
	memcpy(memo, pRecMsg->szMemo, sizeof(pRecMsg->szMemo));

	memset((BYTE*)&hash_node, 0, sizeof(hash_node));
	hash_node.Seq = atoi(trim(seq, strlen(seq)));

	g_COnTimeCtrl.GetAndDeleteOnTimeNode(hash_node);
	memcpy(hash_node.id, pRecMsg->szId, DEVICE_ID_LEN);
	memcpy(hash_node.id+DEVICE_ID_LEN, pRecMsg->szSn, DEVICE_ACTION_SN_LEN);
	memcpy(hash_node.szTime, pRecMsg->szTime, 12);
	hash_node.actTime = g_COnTimeCtrl.getNextActTime(hash_node.szTime);
	strcpy(hash_node.actValue, trim(cdata, strlen(cdata)));

	do
	{
		//添加数据到Task_info表
		const char* sqlFormat = "update task_info set id='%s', sn='%s', ctime='%s', cdata='%s', memo='%s' where seq='%s';";
		char sql[512] = {0};
		sprintf(sql, sqlFormat, trim(id, strlen(id)), trim(sn, strlen(sn)), trim(ctime, strlen(ctime)), trim(cdata, strlen(cdata)), rtrim(memo, strlen(memo)), trim(seq, strlen(seq)));
		if(!g_SqlCtl.ExecSql(sql, NULL, NULL, false, 0))
		{
			break;
		}
		if(0 >= hash_node.actTime)
		{
			ret = SYS_STATUS_SUCCESS;
			break;
		}
		//添加数据到定时任务表
		g_COnTimeCtrl.addOnTimeNode(hash_node);
		ret = SYS_STATUS_SUCCESS;
	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	outLen = 28;

	return ret;
}

//2010 定时任务删除
int Net_Time_Task_Del(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED;
	//业务包指针
	PStrNetTimeTaskDel pRecMsg = (PStrNetTimeTaskDel)inBuf;

	memcpy(outBuf, inBuf, 28);

	ONTIME_ACT_HASH hash_node;
	char seq[11] = {0};
	memcpy(seq, pRecMsg->szSeq, sizeof(pRecMsg->szSeq));

	memset((BYTE*)&hash_node, 0, sizeof(hash_node));
	hash_node.Seq = atoi(trim(seq, strlen(seq)));

	do
	{
		//添加数据到Task_info表
		const char* sqlFormat = "delete from task_info where seq='%s';";
		char sql[512] = {0};
		sprintf(sql, sqlFormat, trim(seq, strlen(seq)));
		if(!g_SqlCtl.ExecSql(sql, NULL, NULL, false, 0))
		{
			break;
		}

		g_COnTimeCtrl.GetAndDeleteOnTimeNode(hash_node);
		ret = SYS_STATUS_SUCCESS;
	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	outLen = 28;

	return ret;
}
/*
//3001设备布防撤防
int Net_Dev_Defence_Update(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED;
	//业务包指针
	PStrNetDevDefenceSet pRecMsg = (PStrNetDevDefenceSet)inBuf;

	memcpy(outBuf, inBuf, 28);

	char id[DEVICE_ID_LEN_TEMP] = {0};
	char cDefence[2] = {0};
	
	memcpy(id, pRecMsg->szDevType, DEVICE_ID_LEN);
	cDefence[0] = pRecMsg->cType;

	do 
	{				
		//撤防，包括采集和动作
		g_DevInfoContainer.HashDeviceDefenceSet(id, atoi(cDefence));
*/
		/*
		//更新到数据库Device_Roll表
		char* sqlFormat = "update device_detail set status = '%s' where id = '%s' and sn = '%s';";
		char sql[512] = {0};
		sprintf(sql, sqlFormat, trim(cDefence, strlen(cDefence)), trim(id, strlen(id)), trim(attrId, strlen(attrId)));
		if(!g_SqlCtl.ExecSql(sql, NULL, NULL, false, 0))
		{
			break;
		}
		*/
/*
		ret = SYS_STATUS_SUCCESS;
	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	outLen = 28;
	return ret;
}
*/
//动作下发
int Net_Dev_Action(const char* inBuf, int inLen, unsigned int seq, int actionSource)
{
	int ret = SYS_STATUS_FAILED;
	//业务包指针
	PStrNetCmd pRecMsg = (PStrNetCmd)inBuf;

	char id[DEVICE_ID_LEN_TEMP] = {0};
	char szActionId[DEVICE_ACTION_SN_LEN_TEMP] = {0};
	char deviceId[DEVICE_ATTR_ID_LEN_TEMP] = {0};
	char szOprator[11] = {0};
	char szActionValue[257] = {0};

	memcpy(deviceId, pRecMsg->szDevType, DEVICE_ATTR_ID_LEN);
	memcpy(id, pRecMsg->szDevType, DEVICE_ID_LEN);
	memcpy(szActionId, pRecMsg->szActionId, DEVICE_ACTION_SN_LEN);

	memcpy(szOprator, pRecMsg->szOprator, 10);
	memcpy(szActionValue, pRecMsg->szActionValue, 256);

	do 
	{		
		ACTION_MSG stActMsg;
		memset((unsigned char*)&stActMsg, 0, sizeof(ACTION_MSG));
		stActMsg.ActionSource = actionSource;
		memcpy(stActMsg.DstId, id, DEVICE_ID_LEN);
		memcpy(stActMsg.ActionSn, szActionId, DEVICE_ACTION_SN_LEN);
		strcpy(stActMsg.Operator, trim(szOprator, strlen(szOprator)));
		strcpy(stActMsg.ActionValue, trim(szActionValue, strlen(szActionValue)));

		time_t nowTime = 0;
		time(&nowTime);
		Time2Chars(&nowTime, stActMsg.szActionTimeStamp);

		//strcpy( stActMsg.szActionTimeStamp, GetNowtimeStr().c_str() );

		stActMsg.Seq = seq;
		ret = CDevInfoContainer::DisPatchAction(stActMsg);
		
		if(SYS_STATUS_SUBMIT_SUCCESS_IN_LIST == ret)
			ret = SYS_STATUS_SUBMIT_SUCCESS;

	} while (false);
	return ret;
}
unsigned int seq_lhy = 0;
int getSeq()
{
	if (0xffffffff == seq_lhy)
	{
		seq_lhy = 0;
	}
	return seq_lhy++;
}
bool Net_Up_Msg(int upCmd, char* buf, int bufLen)
{
	bool ret = false;
	if( g_upFlag == 1 )
	{
		char updateData[4096] = {0};
		memset(updateData, 0, sizeof(updateData));
		MsgHdr* pMsgHdr = (MsgHdr*)updateData;
		pMsgHdr->unMsgLen = MSGHDRLEN + 24 + 4 + bufLen;
		pMsgHdr->unMsgCode = 4;

		PMSG_NET_UPLOAD msgUp = (PMSG_NET_UPLOAD)(updateData + MSGHDRLEN);
		
		sprintf(msgUp->szReserve, "%20d", getSeq());	//保留字
		memset(msgUp->szStatus, 0x30, 4);				//状态
		sprintf(msgUp->szDeal, "%04d", upCmd);
		memcpy(msgUp->szData, buf, bufLen);
		if (!SendMsg_Ex(g_ulHandleNetCliSend, MUCB_IF_NET_CLI_SEND, updateData, pMsgHdr->unMsgLen))
		{
			DBG(("Net_Up_Msg SendMsg_Ex Failed: handle[%d] mucb[%d]\n", g_ulHandleNetCliSend, MUCB_IF_NET_CLI_SEND));
		}
		else
		{
			DBG(("NetApp.cpp 1681 [%d] \n", g_ulHandleNetCliSend));
			ret = true;
		}
	}

	return ret;
}


//3003设备同步
static int DeviceDetailCallBack(void *data, int n_column, char **column_value, char **column_name);    //查询时的回调函数
static int DeviceRollCallBack(void *data, int n_column, char **column_value, char **column_name);    //查询时的回调函数
static int DeviceActionCallBack(void *data, int n_column, char **column_value, char **column_name);    //查询时的回调函数
int g_seq = 0;
int Net_Dev_Syn(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED;
	//业务包指针
	//PStrNetDevSyn pRecMsg = (PStrNetDevSyn)inBuf;

	memcpy(outBuf, inBuf, 28);

	do 
	{				
		//发送回应包
		g_seq = 1;
		const char* sql = "select id, "
					" cname, "
					" brief,"
					" status,"
					" ctype,"
					" private_attr"
					" from device_detail;";
		if(0 != g_SqlCtl.select_from_tableEx((char*)sql, DeviceDetailCallBack, NULL))
		{
			DBG(("DeviceDetailCallBack failed\n"));
			return ret;
		}
	
		Net_Up_Msg(CMD_DEVICE_INFO_UP, (char*)"0000", 4);
		ret = SYS_STATUS_SUCCESS;
	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	outLen = 28;
	return ret;
}
int Net_Dev_Attr_Syn(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED;
	//业务包指针
	//PStrNetDevSyn pRecMsg = (PStrNetDevSyn)inBuf;

	memcpy(outBuf, inBuf, 28);

	do 
	{				
		//发送回应包
		g_seq = 1;
		const char* sql = "select id,"
					" sn,"
					" Attr_Name,"
					" Status,"
					" private_attr"
					" from device_roll;";
		if(0 != g_SqlCtl.select_from_tableEx((char*)sql, DeviceRollCallBack, NULL))
		{
			DBG(("DeviceRollCallBack failed\n"));
			return ret;
		}
	
		Net_Up_Msg(CMD_DEVICE_ATTR_UP, (char*)"0000", 4);
		ret = SYS_STATUS_SUCCESS;
	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	outLen = 28;
	return ret;
}
int Net_Dev_Act_Syn(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED;
	//业务包指针
	//PStrNetDevSyn pRecMsg = (PStrNetDevSyn)inBuf;

	memcpy(outBuf, inBuf, 28);

	do 
	{				
		//发送回应包
		g_seq = 1;
		const char* format = "select device_act.id, "
					" device_act.sn, "
					" device_act.CMDNAME,"
					" device_act.STATUS,"
					" device_act.Private_Attr,"
					" device_cmd.OBJECT "
					" from device_cmd, device_act "
					" where substr(device_act.id,1,%d) = device_cmd.id "
					" and device_act.sn = device_cmd.sn "
					" and device_act.status = '1'"
					" group by device_act.sn, device_act.id;";
		char sql[1024] = {0}; 
		sprintf(sql, format, DEVICE_TYPE_LEN);
		if(0 != g_SqlCtl.select_from_tableEx((char*)sql, DeviceActionCallBack, NULL))
		{
			DBG(("DeviceActionCallBack failed\n"));
			return ret;
		}
	
		Net_Up_Msg(CMD_DEVICE_ACT_UP, (char*)"0000", 4);
		ret = SYS_STATUS_SUCCESS;
	} while (false);
	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	outLen = 28;
	return ret;
}
int DeviceDetailCallBack(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{
	char Id[DEVICE_ID_LEN_TEMP] = {0};
	char cname[21] = {0};
	char brief[21] = {0};
	char status = '0';
	char ctype[3] = {0};
	char private_attr[256] = {0};

	char sendData[1024] = {0};
	memcpy(Id, column_value[0], DEVICE_ID_LEN);		        //设备编号
	strcpy(cname, column_value[1]);							//设备名
	strcpy(brief, column_value[2]);		
	status = column_value[3][0];
	strcpy(ctype, column_value[4]);	
	strcpy(private_attr, column_value[5]);
	if(NULL != strstr(private_attr, "isUpload=1"))
	{
		sprintf(sendData, "%04d%s%20s%20s%c%2s", g_seq++, Id, cname, brief,status, ctype);
		DBG(("%s\n", sendData));		
		Net_Up_Msg(CMD_DEVICE_INFO_UP, sendData, strlen(sendData));
	}
	return 0;
}

int DeviceRollCallBack(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{
	char Id[DEVICE_ID_LEN_TEMP] = {0};
	char sn[DEVICE_SN_LEN_TEMP] = {0};
	char attr_name[21] = {0};
	char status = '0';
	char private_attr[256] = {0};

	char sendData[1024] = {0};
	memcpy(Id, column_value[0], DEVICE_ID_LEN);		        //设备编号
	memcpy(sn, column_value[1], DEVICE_SN_LEN);	
	strcpy(attr_name, column_value[2]);							//设备名
	status = column_value[3][0];
	strcpy(private_attr, column_value[4]);
	if(NULL != strstr(private_attr, "isUpload=1"))
	{
		sprintf(sendData, "%04d%6s%4s%20s%c", g_seq++, Id, sn, attr_name,status);
		DBG(("%s\n", sendData));
		Net_Up_Msg(CMD_DEVICE_ATTR_UP, sendData, strlen(sendData));
	}
	return 0;
}

int DeviceActionCallBack(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{
	char Id[DEVICE_ID_LEN_TEMP] = {0};
	char sn[DEVICE_ACTION_SN_LEN_TEMP] = {0};
	char cmd_name[21] = {0};
	char status = '0';
	char private_attr[256] = {0};
	char object = '0';


	char sendData[1024] = {0};
	memcpy(Id, column_value[0], DEVICE_ID_LEN);		        //设备编号
	memcpy(sn, column_value[1], DEVICE_ACTION_SN_LEN);	
	strcpy(cmd_name, column_value[2]);							//设备名
	status = column_value[3][0];
	strcpy(private_attr, column_value[4]);
	object = column_value[5][0];
	
	if(NULL != strstr(private_attr, "isUpload=1"))
	{
		sprintf(sendData, "%04d%6s%4s%20s%c%c", g_seq++, Id, sn, cmd_name,status, object);
		DBG(("%s\n", sendData));
		Net_Up_Msg(CMD_DEVICE_ACT_UP, sendData, strlen(sendData));
	}
	return 0;
}

//GSM卡IMSI号查询
int Net_GSM_READ_IMSI(const char* inBuf, int inLen, char* outBuf, int& outLen)
{
	int ret = SYS_STATUS_FAILED; 

	//RESERVE+D_STATUS+DEAL
	memcpy(outBuf, inBuf, 28);
	//卡号
	memcpy(outBuf + 28, HardIF_GetGSMIMSI(), 16);
	ret = SYS_STATUS_SUCCESS;

	char szRet[5] = {0};
	sprintf(szRet, "%04d", ret);
	memcpy(outBuf + 20, szRet, 4);
	outLen = 28 + 16;
	return ret;
}
