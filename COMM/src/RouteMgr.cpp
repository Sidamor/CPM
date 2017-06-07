#include "TkMacros.h"   // define macro: TRACE
#include "RouteMgr.h"
#include <assert.h>

CRouteMgr::CRouteMgr()
{
	TRACE(CRouteMgr::CRouteMgr);
	m_bInitialized = false;
	m_phtI2A = NULL;
	m_phtA2I = NULL;
}

CRouteMgr::~CRouteMgr()
{
	TRACE(CRouteMgr::~CRouteMgr);
	if (m_bInitialized)
	{
		pthread_mutex_lock(&m_mtxI2ALock);
		pthread_mutex_lock(&m_mtxA2ILock);
		if (m_phtI2A)
			delete m_phtI2A;
		if (m_phtA2I)
			delete m_phtA2I;
		pthread_mutex_unlock(&m_mtxA2ILock);
		pthread_mutex_destroy(&m_mtxA2ILock);

		pthread_mutex_unlock(&m_mtxI2ALock);
		pthread_mutex_destroy(&m_mtxI2ALock);
	}
}

bool CRouteMgr::Initialize(const int nmb)
{
	TRACE(CRouteMgr::Initialize);
	assert(nmb > 0);
	m_phtI2A = new _I2A_HASH();
	if (!m_phtI2A)
	{
		return false;
	}

	m_phtA2I = new _A2I_HASH();
	if (!m_phtA2I)
	{
		delete m_phtI2A;
		m_phtI2A = NULL;
		return false;
	}

	pthread_mutex_init(&m_mtxI2ALock, NULL);
	pthread_mutex_init(&m_mtxA2ILock, NULL);

	m_bInitialized = true;
	return true;
}

bool CRouteMgr::AddRouteI2A(const int TID, const ClsInetAddress &ia, short int port)
{
	TRACE(CRouteMgr::AddRouteI2A);
	//DBG(("Add TID %d\n", TID));
	assert(m_bInitialized);
	pthread_mutex_lock(&m_mtxI2ALock);

	RouteNode node = {0};
	node.nTID = TID;
	node.addr = ia.GetAddress().s_addr;
	node.port = htons(port);

//	pair<_I2A_HASH::iterator, bool > Rslt = m_phtI2A->insert_unique(node);
	pair<_I2A_HASH::iterator, bool > Rslt = m_phtI2A->insert(_I2A_HASH::value_type(node.nTID, node));

	pthread_mutex_unlock(&m_mtxI2ALock);
//	return Rslt.second;
	return Rslt.second;
}

bool CRouteMgr::AddRouteI2A(const int TID, const TKU32 ia, short int port)
{
	TRACE(CRouteMgr::AddRouteI2A);
	assert(m_bInitialized);
	pthread_mutex_lock(&m_mtxI2ALock);

	RouteNode node = {0};
	node.nTID = TID;
	node.addr = htonl(ia);
	node.port = htons(port);

//	pair<_I2A_HASH::iterator, bool > Rslt = m_phtI2A->insert_unique(node);
	pair<_I2A_HASH::iterator, bool > Rslt = m_phtI2A->insert(_I2A_HASH::value_type(node.nTID, node));

	pthread_mutex_unlock(&m_mtxI2ALock);
//	return Rslt.second;
	return Rslt.second;
}

bool CRouteMgr::DelRouteI2A(const int TID)
{
	TRACE(CRouteMgr::DelRouteI2A);
	//DBG(("Del TID %d\n", TID));
	assert(m_bInitialized);
	pthread_mutex_lock(&m_mtxI2ALock);

	int cnt = m_phtI2A->erase(TID);

	pthread_mutex_unlock(&m_mtxI2ALock);
	return(cnt > 0);
}

bool CRouteMgr::AddRouteA2I(const ClsInetAddress &ia, short int port, const int TID)
{
	TRACE(CRouteMgr::AddRouteA2I);
	assert(m_bInitialized);
	pthread_mutex_lock(&m_mtxA2ILock);

	RouteNode node = {0};
	node.nTID = TID;
	node.addr = ia.GetAddress().s_addr;
	node.port = htons(port);
//	DBG(("addr = %d,\t port = %d, TID = %d\n", (int)node.addr, (int)node.port, TID));
	
	pair<_A2I_HASH::iterator, bool > Rslt = m_phtA2I->insert(_A2I_HASH::value_type(node.addr, node));
	pthread_mutex_unlock(&m_mtxA2ILock);
	return Rslt.second;
}

bool CRouteMgr::AddRouteA2I(const TKU32 ia, short int port, const int TID)
{
	TRACE(CRouteMgr::AddRouteA2I);
	assert(m_bInitialized);
	pthread_mutex_lock(&m_mtxA2ILock);

	RouteNode node = {0};
	node.nTID = TID;
	node.addr = htonl(ia);
	node.port = htons(port);

	pair<_A2I_HASH::iterator, bool > Rslt = m_phtA2I->insert(_A2I_HASH::value_type(node.addr, node));

	pthread_mutex_unlock(&m_mtxA2ILock);
	return Rslt.second;
}

bool CRouteMgr::DelRouteA2I(const ClsInetAddress &ia, short int port)
{
	TRACE(CRouteMgr::DelRouteA2I);
	assert(m_bInitialized);
	bool bRet = false;
	pthread_mutex_lock(&m_mtxA2ILock);

	TKU32 addr = ia.GetAddress().s_addr;
	port = htons(port);

	_A2I_HASH::iterator node = m_phtA2I->find((int)addr);
	while (m_phtA2I->end() != node)
	{
		if (node->second.port == port)
		{
			m_phtA2I->erase(node);
			bRet = true;
			break;
		} else
			++node;
	}

	pthread_mutex_unlock(&m_mtxA2ILock);
	return bRet;
}

bool CRouteMgr::DelRouteA2I(const TKU32 ia, short int port)
{
	TRACE(CRouteMgr::DelRouteA2I);
	assert(m_bInitialized);
	bool bRet = false;
	pthread_mutex_lock(&m_mtxA2ILock);

	unsigned long addr = htonl(ia);
	port = htons(port);

	_A2I_HASH::iterator node = m_phtA2I->find((int)addr);
	while (m_phtA2I->end() != node)
	{
		if (node->second.port == port)
		{
			m_phtA2I->erase(node);
			bRet = true;
			break;
		} else
			++node;
	}

	pthread_mutex_unlock(&m_mtxA2ILock);
	return bRet;
}

// if address returned with struct memory zeroed, means lookup failed.
sockaddr_in CRouteMgr::GetAddrByTID(const int TID)
{
	TRACE(CRouteMgr::GetAddrByTID);
	assert(m_bInitialized);

	sockaddr_in addr = {0};
	pthread_mutex_lock(&m_mtxI2ALock);

	_I2A_HASH::iterator node = m_phtI2A->find(TID);
	if (m_phtI2A->end() != node)
	{
		addr.sin_addr.s_addr = node->second.addr;
		addr.sin_port = node->second.port;
	}

	pthread_mutex_unlock(&m_mtxI2ALock);
	return addr;
}

// return -1, means lookup failed.
int CRouteMgr::GetTIDByAddr(const ClsInetAddress &ia, short int port)
{
	TRACE(CRouteMgr::GetTIDByAddr);
	int nTID = - 1;
	pthread_mutex_lock(&m_mtxA2ILock);

	TKU32 addr = ia.GetAddress().s_addr;
//	port = htons(port);

//	DBG(("addr = %d,\t port = %d\n", (int)addr, (int)port));

	_A2I_HASH::iterator node = m_phtA2I->find((int)addr);
	while (m_phtA2I->end() != node)
	{
		if (node->second.port == port)
		{
			nTID = node->second.nTID;
			break;
		} else
			++node;
	}

	pthread_mutex_unlock(&m_mtxA2ILock);
	return nTID;

}

int CRouteMgr::GetTIDByAddr(const TKU32 ia, short int port)
{
	TRACE(CRouteMgr::GetTIDByAddr);
	int nTID = - 1;
	pthread_mutex_lock(&m_mtxA2ILock);

	unsigned long addr = htonl(ia);
	port = htons(port);

	_A2I_HASH::iterator node = m_phtA2I->find((int)addr);
	while (m_phtA2I->end() != node)
	{
		if (node->second.port == port)
		{
			nTID = node->second.nTID;
			break;
		} else
			++node;
	}

	pthread_mutex_unlock(&m_mtxA2ILock);
	return nTID;

}




