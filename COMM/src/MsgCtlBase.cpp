#include "MsgCtlBase.h"

CMsgCtlBase::CMsgCtlBase()
{
	m_unHandle = 0;
	funcMapCount = 0;
	m_thread = 0xFFFF;
	m_bInitialized = false;
}   
//destructor
CMsgCtlBase::~CMsgCtlBase(void)
{
	if(0xFFFF != m_thread)
	{
		ThreadPool::Instance()->SetThreadMode(m_thread, false);
		pthread_cancel(m_thread);
	}
}

int CMsgCtlBase::AddFunc(DWORD code, CMDFUNC func)
{
	assert(true == m_bInitialized);
	assert (funcMapCount < FUNC_TAB_SIZE);
	funcMap[funcMapCount].code = code;
	funcMap[funcMapCount].func = func;
	funcMapCount++;
	return 0;
}


bool CMsgCtlBase::Initialize(TKU32 unInputMask,const char *szCtlName)
{
	char buf[256] = {0};
	sprintf(buf, "%s %s", szCtlName, "Controlor\n");

	g_MsgMgr.Attach(unInputMask, m_unHandle);
//	DBG(("CMsgCtlBase initializing m_unHandle = %d\n", m_unHandle));
	if (!ThreadPool::Instance()->CreateThread(buf, CMsgCtlBase::pThrdCtl, true,this))
	{
		return false;
	}
	m_bInitialized = true;
	return true;
}

void * CMsgCtlBase::pThrdCtl(void *arg)
{
	CMsgCtlBase *_this = (CMsgCtlBase *)arg;
	_this->MsgCtlFunc();
	return NULL;
}
void CMsgCtlBase::MsgCtlFunc(void)
{
	QUEUEELEMENT pMsg ;
	CMDFUNC func;
	MsgHdr *hdr;

	m_thread = pthread_self();
	
	while(1)
	{
		pthread_testcancel();
		
		if (MMR_OK == g_MsgMgr.GetFirstMsg(m_unHandle, pMsg, 1000))
		{
			if (pMsg == NULL) continue;
			hdr = (MsgHdr*)pMsg;
			//DBG(("--------Recieve----------ListHandle[%d]--",m_unHandle));
			//AddMsg((BYTE *)pMsg, hdr->unMsgLen);
			func = NULL;
			
			for (int i = 0; i < funcMapCount; i++)
			{
				//DBG(("hdr->unMsgCode=%d\n",hdr->unMsgCode));
				if (funcMap[i].code == hdr->unMsgCode)
				{
					func = funcMap[i].func;
					break;
				}
			}
			
			if (func != NULL)
			{
				func(this, (char *)pMsg );
			}
			MM_FREE(pMsg );
		}
	}
}
