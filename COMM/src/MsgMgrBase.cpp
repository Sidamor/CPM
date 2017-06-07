#include "MsgMgrBase.h"

FreeMemEvent g_QEMHandler = NULL;

CMsgMgrBase::CMsgMgrBase()
{
	TRACE(CMsgMgrBase::CMsgMgrBase);
	m_pIdxArray = NULL;
	m_pQueueArray = NULL;
	m_Initialized = false;
	m_StateArray = NULL;
	m_pQIDStack = NULL;
}

CMsgMgrBase::~CMsgMgrBase(void)
{
	TRACE(CMsgMgrBase::~CMsgMgrBase);
	if (m_pIdxArray)
		delete[] m_pIdxArray;
	if (m_pQueueArray)
		delete[] m_pQueueArray;
	if (m_StateArray)
		delete[] m_StateArray;
	if (m_pQIDStack)
		delete m_pQIDStack;
	pthread_mutex_unlock(&m_mtxTnlLock);
	pthread_mutex_destroy(&m_mtxTnlLock);
}

bool CMsgMgrBase::Initialize(int nQueueCnt)
{
	TRACE(CMsgMgrBase::Initialize);
	assert(false == m_Initialized);
	assert(nQueueCnt > 0);
	m_nMaxQueueCnt = nQueueCnt;
	m_pIdxArray = new  TKU32[m_nMaxQueueCnt];
	if (!m_pIdxArray)
		return false;
	m_pQueueArray = new CPtrQueueMT[m_nMaxQueueCnt];
	if (!m_pQueueArray)
		return false;
	m_StateArray = new bool[m_nMaxQueueCnt];
	if (!m_StateArray)
		return false;
	m_pQIDStack = new CFIStack(m_nMaxQueueCnt);
	if (!m_pQIDStack)
		return false;
	for (int i = m_nMaxQueueCnt - 1; i > -1; i--)		//lsd 2013-11-5
	{
		size_t tId = i;
		m_pQIDStack->Push(tId);
		m_StateArray[tId] = false;
		m_pIdxArray[tId] = 0;
	}

	memset(&m_mtxTnlLock, 0, sizeof(m_mtxTnlLock));
	pthread_mutex_init(&m_mtxTnlLock, NULL);
	m_Initialized = true;

	return true;
}


//申请消息通道，nTID为返回的消息通道号
MMRType CMsgMgrBase::AllocTunnel(TKU32 unMask, TKU32 unQueueLen, 
								 size_t& nTID, FreeMemEvent hdl)
{
	TRACE(CMsgMgrBase::AllocTunnel);
	pthread_mutex_lock(&m_mtxTnlLock);

	if (false == m_pQIDStack->Pop(nTID))
	{
		pthread_mutex_unlock(&m_mtxTnlLock);
		return MMR_VOLUME_LIMIT;
	}

	if (false == m_pQueueArray[nTID].Initialize(unQueueLen, hdl))
	{
		m_pQIDStack->Push(nTID);
		pthread_mutex_unlock(&m_mtxTnlLock);
		return MMR_GEN_ERR;
	}

	m_pIdxArray[nTID] = unMask;
	m_StateArray[nTID] = true;
	pthread_mutex_unlock(&m_mtxTnlLock);
	return MMR_OK;
}

//释放消息通道，nTID为消息通道号
MMRType CMsgMgrBase::FreeTunnel(int nTID)
{
	TRACE(CMsgMgrBase::FreeTunnel);
	pthread_mutex_lock(&m_mtxTnlLock);
	if ((nTID < 0) || (nTID >= m_nMaxQueueCnt))
	{
		pthread_mutex_unlock(&m_mtxTnlLock);
		return MMR_GEN_ERR;
	}
	if (!m_StateArray[nTID])
	{
		pthread_mutex_unlock(&m_mtxTnlLock);
		return MMR_TUNNEL_NOT_EXIST;
	}
	if (false == m_pQIDStack->Push(nTID))
	{
		pthread_mutex_unlock(&m_mtxTnlLock);
		return MMR_VOLUME_LIMIT;
	}

	m_pQueueArray[nTID].Finalize();
	m_pIdxArray[nTID] = 0;
	m_StateArray[nTID] = false;
	pthread_mutex_unlock(&m_mtxTnlLock);
	return MMR_OK;
}


MMRType CMsgMgrBase::Attach(TKU32 unMask, TKU32& unHandle)
{
	TRACE(CMsgMgrBase::Attach);
	assert(true == m_Initialized);
	pthread_mutex_lock(&m_mtxTnlLock);
	for (int i = 0; i < (int)m_nMaxQueueCnt; i++)
	{
		if ((unMask == m_pIdxArray[i]) && (m_StateArray[i]))
		{
			unHandle  = i;
			pthread_mutex_unlock(&m_mtxTnlLock);
			return MMR_OK;
		}
	}
	pthread_mutex_unlock(&m_mtxTnlLock);
	return MMR_TUNNEL_NOT_EXIST;
}

MMRType CMsgMgrBase::SendMsg(TKU32 unHandle, QUEUEELEMENT qeMsg, int nTimeout)
{
	//TRACE(CMsgMgrBase::SendMsg);
	assert(true == m_Initialized);
	if ((unHandle < (TKU32)m_nMaxQueueCnt) && m_StateArray[unHandle])
	{
		if (true == m_pQueueArray[unHandle].AddTail(qeMsg, nTimeout))
		{
			//DBG(("CMsgMgrBase::SendMsg >%d\n",(int)unHandle));
			return MMR_OK;
		} 
		else
			return MMR_GEN_ERR;
	} 
	else
		return MMR_TUNNEL_NOT_EXIST;
}

MMRType CMsgMgrBase::InsertMsg(TKU32 unHandle, QUEUEELEMENT qeMsg, int nTimeout)
{
//	TRACE(CMsgMgrBase::InsertMsg);
	assert(true == m_Initialized);
	if ((unHandle < (TKU32)m_nMaxQueueCnt) && m_StateArray[unHandle])
	{
		if (true == m_pQueueArray[unHandle].AddHead(qeMsg, nTimeout))
		{
//			DBG(("CMsgMgrBase::InsertMsg >%d\n",(int)unHandle));
			return MMR_OK;
		}
		else
			return MMR_GEN_ERR;
	} else
		return MMR_TUNNEL_NOT_EXIST;
}

MMRType CMsgMgrBase::GetFirstMsg(TKU32 unHandle, QUEUEELEMENT &qeMsg, int nTimeout)
{
//	TRACE(CMsgMgrBase::GetFirstMsg);
	assert(true == m_Initialized);
	if ((unHandle < (TKU32)m_nMaxQueueCnt)  && m_StateArray[unHandle])
	{
//		DBG(("CMsgMgrBase::GetFirstMsg start <%d\n",(int)unHandle));
		if (true == m_pQueueArray[unHandle].RemoveHead(qeMsg, nTimeout))
		{
//			DBG(("CMsgMgrBase::GetFirstMsg <%d\n",(int)unHandle));
			return MMR_OK;
		}
		else
			return MMR_GEN_ERR;
	} else
		return MMR_TUNNEL_NOT_EXIST;
}

MMRType CMsgMgrBase::GetLastMsg(TKU32 unHandle, QUEUEELEMENT& qeMsg, int nTimeout)
{
//	TRACE(CMsgMgrBase::GetLastMsg);
	assert(true == m_Initialized);
	if ((unHandle < (TKU32)m_nMaxQueueCnt)  && m_StateArray[unHandle])
	{
		if (true == m_pQueueArray[unHandle].RemoveTail(qeMsg, nTimeout))
		{
//			DBG(("CMsgMgrBase::GetLastMsg <%d\n",(int)unHandle));
			return MMR_OK;
		}
		else
			return MMR_GEN_ERR;
	} else
		return MMR_TUNNEL_NOT_EXIST;
}

MMRType CMsgMgrBase::GotoHead(TKU32 unHandle)
{
	assert(true == m_Initialized);
	if ((unHandle < (TKU32)m_nMaxQueueCnt)  && m_StateArray[unHandle])
	{
		m_pQueueArray[unHandle].GotoHead();
		return MMR_OK;
	} else
		return MMR_TUNNEL_NOT_EXIST;
}
MMRType CMsgMgrBase::PeerNext(TKU32 unHandle, QUEUEELEMENT& qeMsg)
{
	assert(true == m_Initialized);
	if ((unHandle < (TKU32)m_nMaxQueueCnt)  && m_StateArray[unHandle])
	{
		if (true == m_pQueueArray[unHandle].PeerNext(qeMsg))
			return MMR_OK;
		else
			return MMR_PEER_EOQ;
	}
	return MMR_TUNNEL_NOT_EXIST;
}


MMRType CMsgMgrBase::ConsumerAllowed(TKU32 unHandle, bool isallow)
{
	if ((unHandle < (TKU32)m_nMaxQueueCnt)  && m_StateArray[unHandle])
	{
		m_pQueueArray[unHandle].m_bConsumerAllowed = isallow;
		return MMR_OK;
	}
	else
		return MMR_TUNNEL_NOT_EXIST;		
}

