#ifndef _SO_IF_H_
#define _SO_IF_H_

#include "Define.h"

#pragma pack(1)

typedef struct _SCH_CARD_ID
{
	char DevId[DEVICE_ATTR_ID_LEN_TEMP];         //…Ë±∏±‡∫≈ 
	char Id[MAX_PACKAGE_LEN_TERMINAL];
}SCH_CARD_ID, *PSCH_CARD_ID;


bool SOIF_Init();
bool SOIF_Terminate();

int SOIF_PollProc(int m_iChannelId, PDEVICE_INFO_POLL device_node, unsigned char* respBuf, int& nRespLen);





#endif
