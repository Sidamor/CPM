/******************************************************************************/
/*    File    : SOIF.cpp   VERSION:CPM_V4	                                   */
/*    Author  :                                                               */
/*    Creat   :                                                               */
/*    Function:                                                               */
/******************************************************************************/

/******************************************************************************/
/*                       Í·ÎÄ¼þ                                               */
/******************************************************************************/
#include <sys/time.h>
#include <sys/ioctl.h>
#include <string>
#include <stdlib.h>
#include <error.h>
#include "SOIF.h"

#include "HardIF.h"
#include "IniFile.h"


#ifdef  DEBUG
//#define DEBUG_SOIF
#endif

#ifdef DEBUG_SOIF
#define DBG_SOIF(a)		printf a;
#else
#define DBG_SOIF(a)	
#endif



bool SOIF_Init()
{
	return true;
}

bool SOIF_Terminate()
{
	return true;
}

int SOIF_PollProc(int m_iChannelId, PDEVICE_INFO_POLL device_node, unsigned char* respBuf, int& nRespLen)
{
	return 0;
}





