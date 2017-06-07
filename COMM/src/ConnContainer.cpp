#include "Stack.h"
#include "NetIfBase.h"
#include "ThreadPool.h"
#include "ConnContainer.h"

ConnContainer::ConnContainer()
{
	TRACE(ConnContainer::ConnContainer);
	m_IdStack = NULL;
	m_pConnHash = NULL;
	m_thread = 0xFFFF;
	pthread_mutex_init(&m_mtxHashLock, NULL);
}

ConnContainer::~ConnContainer()
{
	TRACE(ConnContainer::~ConnContainer);
	CNetIfBase* ifBase = NULL;
	pthread_mutex_lock(&m_mtxHashLock);

	if(0xFFFF != m_thread)
		pthread_cancel(m_thread);	
	
	while (m_IdStack->Pop((size_t&)ifBase))
	{
		delete ifBase;
	}
	if (m_IdStack)
		delete m_IdStack;

	if(m_pConnHash)
	{
		for (_CONN_HASH::iterator conn = m_pConnHash->begin(); conn != m_pConnHash->end(); conn++)
		{
			try
			{
				if(conn->second.ifBase)
					delete conn->second.ifBase;
			}
			catch(...)
			{
			}
		}
		delete m_pConnHash;
	}

	pthread_mutex_unlock(&m_mtxHashLock);
	pthread_mutex_destroy(&m_mtxHashLock);
}

bool ConnContainer::Initialize(int nConnCnt)
{
	TRACE(ConnContainer::Initialize);
	assert(false == m_bIsInitialized);
	assert(nConnCnt > 0);
	m_IdStack = new CFIStackMT(nConnCnt);//new CFIStack(nConnCnt);
	if (!m_IdStack)
		return false;
	m_pConnHash = new _CONN_HASH();
	if (!m_pConnHash)
	{
		delete m_IdStack;
		m_IdStack = NULL;
		return false;
	}

	char buf[256] = "ConnContainer::pThrdScanner";
	if (!ThreadPool::Instance()->CreateThread(buf, ConnContainer::pThrdScanner, true, this))
	{
		TRACE("ConnContainer::pThrdScanner start failed.\n");
		delete m_pConnHash;
		delete m_IdStack;
		m_pConnHash = NULL;
		m_IdStack = NULL;
		return false;
	}

	m_bIsInitialized = true;
	return true;
}

void* ConnContainer::pThrdScanner(void *pArg)
{

	TRACE(ConnContainer::pThrdScanner);
	ConnContainer*  _this = (ConnContainer *)pArg;
	_this->m_thread = pthread_self();
	
	while (1)
	{
		pthread_testcancel();
		
		CNetIfBase *ifBase = NULL;
		//pthread_mutex_lock(&_this->m_mtxHashLock);
		try
		{
			while (g_ConnContainer.m_IdStack->Pop((size_t&)ifBase))
			{
static int total = 0;
++total;
printf("ConnContainer ThrdScanner delete ifBase [%d] \n", total);
				delete ifBase;
//ifBase->Terminate();
			}
		} 
		catch (...)
		{
DBGLINE
		}

		//pthread_mutex_unlock(&_this->m_mtxHashLock);
//DBG(("g_ConnContainer do sleep\n"));
		sleep(1);
	}
	return NULL;
}

void ConnContainer::OnLinkDown(int nIfId)
{
	//查找哈希表，修改状态
	//如果找到表项，执行下列语句
	TRACE(ConnContainer::OnLinkDown);
	try
	{	
		CNetIfBase* pNetIf = NULL;
		pthread_mutex_lock(&g_ConnContainer.m_mtxHashLock);
		_CONN_HASH::iterator conn = g_ConnContainer.m_pConnHash->find(nIfId);
		if (g_ConnContainer.m_pConnHash->end() != conn)
			pNetIf =  conn->second.ifBase;
		pthread_mutex_unlock(&g_ConnContainer.m_mtxHashLock);
		if (pNetIf)
			conn->second.connMgr->OnLinkDown(pNetIf);
	} 
	catch (...)
	{
DBGLINE
	}
}

bool ConnContainer::AddConn(ClsConnMgr* mgr, CNetIfBase* ifBase)
{
	TRACE(ConnContainer::AddConn);
	assert(true == m_bIsInitialized);

	pthread_mutex_lock(&m_mtxHashLock);
	ConnInfo conn;
	conn.nKey = ifBase->GetSockId();
	conn.connMgr = mgr;
	conn.ifBase = ifBase;
	//向哈希表加入conn

	pair<_CONN_HASH::iterator, bool >  Rslt = m_pConnHash->insert(_CONN_HASH::value_type(conn.nKey, conn));
	if(Rslt.second)
	{
		//DBG(("insert_unique(conn) %d\n", conn.nKey));
	}
		
	m_pConnHash->insert(_CONN_HASH::value_type(conn.nKey, conn));

	pthread_mutex_unlock(&m_mtxHashLock);
	return Rslt.second;
}

bool ConnContainer::RemoveConn(int nIfId, bool bDelIfNow)
{
	TRACE(ConnContainer::RemoveConn);
	assert(true == m_bIsInitialized);

	//DBG(("ConnContainer::RemoveConn %d\n", nIfId));
	bool bRet = true;
//printf("before lock \n");
	pthread_mutex_lock(&m_mtxHashLock);
//printf("after lock \n");
	_CONN_HASH::iterator conn = m_pConnHash->find(nIfId);
	//DBG(("Found nIfId to remove\n"));
	if (m_pConnHash->end() != conn)
	{
		if (NULL != conn->second.ifBase)
		{
			if (bDelIfNow)
			{
				DBG(("ConnContainer::delete ifBase\n"));
				delete conn->second.ifBase;
			} 
			else
			{
				m_IdStack->Push((size_t)conn->second.ifBase);
				//delete conn->ifBase;
static int total = 0;
++total;
printf("ConnContainer::Push ifBase [%d] \n", total);

			}
			m_pConnHash->erase(conn);
		}
		bRet = false;
	}
	bRet = false;
	pthread_mutex_unlock(&m_mtxHashLock);
	return bRet;
}





