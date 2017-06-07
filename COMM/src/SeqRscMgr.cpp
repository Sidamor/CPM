#include <memory.h>
#include <string>
#include <assert.h>
#include <stdio.h>
#include "SeqRscMgr.h"
using namespace std;
CSeqRscMgr g_SeqRscMgr;


CSeqRscMgr::CSeqRscMgr(void)
{
	m_KeyArray = NULL;
	m_SeqArray = NULL;
	m_RscState = NULL;
	m_nUsedCnt = 0;
	m_nRscCnt = 0;
	m_bInitialized = false;
	OnGetVal = NULL;
	OnGetKey = NULL;
	memset(&m_tmLock, 0, sizeof(pthread_mutex_t));
	pthread_mutex_init(&m_tmLock, NULL);
}

CSeqRscMgr::~CSeqRscMgr(void)
{
	pthread_mutex_lock(&m_tmLock);
	if (m_KeyArray)
	{
		delete[] m_KeyArray;
	}
	if (m_SeqArray)
	{
		delete[] m_SeqArray;
	}
	if (m_RscState)
	{
		delete[] m_RscState;
	}
	pthread_mutex_unlock(&m_tmLock);
	pthread_mutex_destroy(&m_tmLock);
}

bool CSeqRscMgr::Initialize(int nNmb, GetValEvent hGetVal, GetKeyEvent hGetKey)
{
	bool bRet = true;
	assert(false == m_bInitialized);
	pthread_mutex_lock(&m_tmLock);
	if ((NULL == hGetVal) || (NULL == hGetKey) || (nNmb < 1))
	{
		bRet = false;
	} else
	{
		m_nRscCnt = nNmb;
		OnGetVal = hGetVal;
		OnGetKey = hGetKey;
		try
		{
			m_KeyArray = new int[m_nRscCnt];
			m_SeqArray = new SEQNO[m_nRscCnt];
			m_RscState = new bool[m_nRscCnt];
			if (0 == ((long)m_KeyArray & (long)m_SeqArray & (long)m_RscState))
			{
				bRet = false;
			} else
			{
				for (int i = 0; i < m_nRscCnt; i++)
				{
					m_KeyArray[i] = i;
					m_SeqArray[i] = OnGetVal(i);
					m_RscState[i] = false;
				}
			}
		} catch (...)
		{
			bRet = false;
		}
	}
	m_bInitialized = bRet;
	pthread_mutex_unlock(&m_tmLock);
	return bRet;
}


bool CSeqRscMgr::Alloc(int& nVal)
{
	bool bRet = true;
	assert(true == m_bInitialized);
	assert(m_nUsedCnt >= 0);
	pthread_mutex_lock(&m_tmLock);
	if (m_nUsedCnt < m_nRscCnt)
	{
		int nKey = m_KeyArray[m_nUsedCnt];
		nVal = m_SeqArray[nKey];
		m_RscState[nKey] = true;
		m_nUsedCnt++;
	} else
	{
		bRet = false;
	}
	pthread_mutex_unlock(&m_tmLock);
	return bRet;
}

bool CSeqRscMgr::Free(const SEQNO val)
{
	bool bRet = true;
	assert(true == m_bInitialized);
	assert(m_nUsedCnt <= m_nRscCnt);
	pthread_mutex_lock(&m_tmLock);
	if (m_nUsedCnt > 0)
	{
		int nKey = -1;
		try
		{
			nKey = OnGetKey(val);
		} catch (...)
		{
			nKey = -1;
		}

		if ((nKey >= 0) && (nKey < m_nRscCnt) && (true == m_RscState[nKey]))
		{
			m_nUsedCnt--;
			m_KeyArray[m_nUsedCnt] = nKey;
			m_RscState[nKey] = false;
		} else
		{
			bRet = false;
		}
	} else
	{
		bRet = false;
	}
	pthread_mutex_unlock(&m_tmLock);
	return bRet;
}

bool CSeqRscMgr::IsInUse(int& nKey)
{
	assert(true == m_bInitialized);
	if ((nKey >= 0) && (nKey < m_nRscCnt))
	{
		return m_RscState[nKey];
	} else
	{
		nKey = -1;
		return false;
	}
}

char* CSeqRscMgr::sPrintRscInUse(char* buff, int size)
{
	assert(size > 0);
	assert(true == m_bInitialized);
	int nBytes = 0;
	for (int i = 0; i < m_nRscCnt; i++)
	{
		if (true == m_RscState[i])
		{
			nBytes += snprintf(buff + nBytes, size, "Key = %d		Value = %d\n", i, m_SeqArray[i]);
			if (nBytes < 0)
			{
				break;
			}
		}
	}
	buff[size - 1] = 0;
	return buff;
}
