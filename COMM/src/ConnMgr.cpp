#include "ConnContainer.h"
#include "ConnMgr.h"

ClsConnMgr::ClsConnMgr(void)
{
		memset(&m_info, 0, sizeof(ModIfInfo));
//		m_nId = -1;
	}
	ClsConnMgr::~ClsConnMgr(void)
{
//		m_nId = -1;
	}
//ClsCltConnMgr
ClsCltConnMgr::~ClsCltConnMgr(void)
{
	TRACE(ClsCltConnMgr::~ClsCltConnMgr);
}

void  ClsCltConnMgr::SetModIfInfo(ModIfInfo& info)
{
	TRACE(ClsCltConnMgr::SetModIfInfo);
	ClsConnMgr::SetModIfInfo(info);
	info.bTcpIsSvrConn = false;
}

bool ClsCltConnMgr::CreateConn(void)
{
	//调用本方法之前执行SetModInfo
	TRACE(ClsSvrConnMgr::CreateConn);
	CNetIfBase* ifBase = new CNetIfBase(m_info, ConnContainer::OnLinkDown);
	if (ifBase)
	{
		if (ifBase->Initialize())
		{
			if (false == g_ConnContainer.AddConn(this, ifBase))
			{
				delete ifBase;
				return false;
			}
			return true;
		} 
		else
		{
			delete ifBase;
			return false;
		}
	}
	else
	{
		return false;
	}
	return true;
}

bool ClsCltConnMgr::DropConn(int nSocket)
{
	TRACE(ClsSvrConnMgr::DropConn);
	return g_ConnContainer.RemoveConn(nSocket);
}


//ClsSvrConnMgr

ClsSvrConnMgr::~ClsSvrConnMgr(void)
{
	TRACE(ClsSvrConnMgr::~ClsSvrConnMgr);
}

bool ClsSvrConnMgr::ListenAt(ClsInetAddress& ia,  tpport_t port)
{
	TRACE(ClsSvrConnMgr::ListenAt);
	if (SetServerSocket(ia, port))
	{
		if (0 == Listen())
		{
			if (Run(AM_INHERIT))
			{
				return true;
			}
			return false;
		}
		return false;
	}
	return false;
}

void  ClsSvrConnMgr::SetModIfInfo(ModIfInfo& info, CNetIfBase* pNetIf)
{
	TRACE(ClsSvrConnMgr::SetModIfInfo);
	ClsConnMgr::SetModIfInfo(info);
	info.bTcpIsSvrConn = true;
	m_pNetIf = pNetIf;
	if (m_pNetIf)
		m_pNetIf->pSvrConnMgr = this;
}

bool ClsSvrConnMgr::OnAccept(int nSocket)
{
	TRACE(ClsSvrConnMgr::OnAccept);
	if (m_info.npType==NP_TCPSVR)
	{
		assert(m_pNetIf!=NULL);
		return m_pNetIf->SetClient(nSocket);
	}
	m_info.nTcpSocket = nSocket;
	CNetIfBase* ifBase = new CNetIfBase(m_info, ConnContainer::OnLinkDown);
	if (ifBase)
	{
		if (ifBase->Initialize())
		{

			if (false == g_ConnContainer.AddConn(this, ifBase))
			{
DBGLINE
				delete ifBase;
				return false;
			}
static int total = 0;
++total;
printf("ClsSvrConnMgr::OnAccept [%d] \n", total);
		} 
		else
		{
DBGLINE
			return false;
		}
	} else
	{
DBGLINE	
		return false;
	}
	return true;
}

void ClsSvrConnMgr::OnLinkDown(CNetIfBase* ifBase)
{
static int total = 0;
++total;
printf("ClsSvrConnMgr::OnLinkDown [%d] \n", total);
//return;

	TRACE(ClsSvrConnMgr::OnLinkDown);
	if (NULL != ifBase)
	{
		bool bRet = g_ConnContainer.RemoveConn(ifBase->GetIfId(), false);
		if (bRet)
		{
			ReduceConn();
		} else
		{
			//向OMS报警
		}
	}
}







