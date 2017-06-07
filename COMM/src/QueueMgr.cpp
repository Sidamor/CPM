#include "QueueMgr.h"



/*****************************************************************************
	方法名: CPtrQueueMT
	参数:   [in] unQueueLen	---- 队列长度
	返回值: 无

	说明:	构造函数，本方法在CPtrQueueMT类实例被创建时调用，
			完成初始化队列控制信号灯，队列头尾指针和队列内存的任务
*****************************************************************************/
CPtrQueueMT::CPtrQueueMT()
{
	Init();
}

void CPtrQueueMT::Init()
{
	// 初始化头尾指针
	m_nHead = 0;
	m_nTail = 0;
	m_nElemCnt = 0;
	m_nCursor = -1;
	OnRelMem = NULL;
	m_bInitialized = false;
	m_bProducerAllowed = false;
	m_bConsumerAllowed = false;
	memset(&m_hSemEmptyLck, 0, sizeof(sem_t));
	memset(&m_hSemFullLck, 0, sizeof(sem_t));
	memset(&m_hHeadLck, 0, sizeof(sem_t));
	memset(&m_hTailLck, 0, sizeof(sem_t));
}
/*****************************************************************************
	方法名: Initialize
	参数:  [in] unQueueLen	---- 队列长度
	返回值:	true		成功
				false		失败

	说明:	本方法在CPtrQueueMT类实例被创建后调用，
			完成初始化队列控制信号灯，队列头尾指针和队列内存的任务
*****************************************************************************/
bool CPtrQueueMT::Initialize(TKU32 unQueueLen, FreeMemEvent hRelMem)
{
	Finalize();
	Init();
	OnRelMem = hRelMem;
	memset(&m_hSemEmptyLck, 0, sizeof(sem_t));
	m_hSemEmptyLck.__align = unQueueLen;
	if (-1 == sem_init(&m_hSemEmptyLck, 0, unQueueLen))
	{
		return false;
	}

	memset(&m_hSemFullLck, 0, sizeof(sem_t));
	m_hSemFullLck.__align = 0;
	if (-1 == sem_init(&m_hSemFullLck, 0, (TKU32)0))
	{
		return false;
	}

	int nVal = 0;
	sem_getvalue(&m_hSemFullLck, &nVal);


	memset(&m_hHeadLck, 0, sizeof(sem_t));
	m_hHeadLck.__align = 1;
	if (-1 == sem_init(&m_hHeadLck, 0, (TKU32)1))
	{
		return false;
	}

	memset(&m_hTailLck, 0, sizeof(sem_t));
	m_hTailLck.__align = 1;
	if (-1 == sem_init(&m_hTailLck, 0, (TKU32)1))
	{
		return false;
	}

	// 申请指针队列内存

	m_unQueueLen = unQueueLen;
	m_ptrQueue = NULL;
	m_ptrQueue = (QUEUEELEMENT *)new char[sizeof(QUEUEELEMENT) * unQueueLen];
	if (!m_ptrQueue)
		return false;

	m_bProducerAllowed = true;
	m_bConsumerAllowed = true;
	m_bInitialized = true;
	return true;
}

/*****************************************************************************
方法名: Finalize
参数:	无
返回值: 无

说明:	清理函数，本方法在CPtrQueueMT类实例被删除时被析构函数调用，
		完成释放队列控制信号灯句柄和队列内存的任务
*****************************************************************************/
void CPtrQueueMT::Finalize(void)
{
	if (true == m_bInitialized)
	{
		m_bProducerAllowed = false;
		m_bConsumerAllowed = false;
		sem_wait(&m_hHeadLck);
		sem_wait(&m_hTailLck);
		sem_destroy(&m_hHeadLck);
		sem_destroy(&m_hTailLck);
		sem_destroy(&m_hSemEmptyLck);
		sem_destroy(&m_hSemFullLck);

		if (NULL != OnRelMem)
			try
			{
				while (m_nElemCnt-- > 0)
				{
					OnRelMem(m_ptrQueue[m_nHead]);
					m_nHead = (m_nHead + 1) % m_unQueueLen;
				}
			} catch (...)
			{
			}

		delete[] m_ptrQueue;

		m_bInitialized = false;
	}
}

/*****************************************************************************
	方法名: ~CPtrQueueMT
	参数:   无
	返回值: 无
	
	说明:	析构函数，本方法在CPtrQueueMT类实例被删除时调用，
			完成释放队列控制信号灯句柄和队列内存的任务
*****************************************************************************/
CPtrQueueMT::~CPtrQueueMT(void)
{
	Finalize();
}

/*****************************************************************************
	方法名: PrimP
	参数:   
			[in] hSem      ---- 信号量句柄
			[in] nTimeout ---- 超时
	返回值:
			SEM_PICKED    ---- 取得一个信号量
			SEM_TIOMEOUT  ---- 超时
			SEM_ERROR     ---- 出错

	说明:   本方法实现经典系统理论中的P操作,
			具体可看教科书.
*****************************************************************************/
int CPtrQueueMT::PrimP(sem_t& hSem, int nTimeout)
{
	assert(nTimeout >= TW_INFINITE);
	if (TW_INFINITE == nTimeout)
	{
		return sem_wait(&hSem);
	} else if (0 == nTimeout)
	{
		return sem_trywait(&hSem);
	} else
	{
		return SemWait(&hSem, nTimeout);
	}
}

/*****************************************************************************
	方法名: PrimV
	参数:   [in] hSem      ---- 信号量句柄
	返回值:
			true   ---- 成功
			false  ---- 失败

	说明:   本方法实现经典系统理论中的V操作,
			具体可看教科书.

*****************************************************************************/
int CPtrQueueMT::PrimV(sem_t& hSem)
{
	return sem_post(&hSem);
}

/*****************************************************************************
	方法名: AddHead
	参数:   
			[in] ptrObj     ---- 待添加的指针
			[in] nTimeout  ---- 操作超时时长
	返回值:
			true   ---- 成功
			false  ---- 失败

	说明:   本函数实现向队列头部添加一个指针，多线程安全

*****************************************************************************/
bool CPtrQueueMT::AddHead(QUEUEELEMENT ptrObj, int nTimeout)
{
	if (false == m_bProducerAllowed)
		return false;
	if (PrimP(m_hSemEmptyLck, nTimeout) != 0)
		return false;
	if (PrimP(m_hHeadLck, TW_INFINITE) != 0)
	{
		PrimV(m_hSemEmptyLck);
		return false;       
	}

	//回滚头指针
	m_nHead = (m_nHead - 1 + m_unQueueLen) % m_unQueueLen;

	//将一个指针加入队列
	m_ptrQueue[m_nHead] = ptrObj;
	m_nElemCnt++;

	if (PrimV(m_hHeadLck) != 0)
		return false;

	if (PrimV(m_hSemFullLck) != 0)
		return false;

	return true;    
}


/*****************************************************************************
	方法名: AddTail
	参数:   
			[in] ptrObj     ---- 待添加的指针
			[in] nTimeout  ---- 操作超时时长
	返回值:
			true   ---- 成功
			false  ---- 失败

	说明:   本函数实现向队列尾部添加一个指针，多线程安全

*****************************************************************************/
bool CPtrQueueMT::AddTail(QUEUEELEMENT ptrObj, int nTimeout)
{
	if (false == m_bProducerAllowed)
		return false;
	if (PrimP(m_hSemEmptyLck, nTimeout) != 0)
		return false;
	if (PrimP(m_hTailLck, TW_INFINITE) != 0)
	{
		PrimV(m_hSemEmptyLck);
		return false;       
	}
	//将一个指针加入队列
	m_ptrQueue[m_nTail] = ptrObj;
	//修改尾指针
	m_nTail = (m_nTail + 1) % m_unQueueLen;
	m_nElemCnt++;

	if (PrimV(m_hTailLck) != 0)
		return false;

	if (PrimV(m_hSemFullLck) != 0)
		return false;
	return true;    
}


/*****************************************************************************
	方法名: RemoveHead
	参数:   
			[out] ptrObj	---- 待取得的指针
			[in]  nTimeout	---- 操作超时时长
	返回值:
			true   ---- 成功
			false  ---- 失败

	说明:   本方法实现从队列首部取得一个指针，多线程安全

*****************************************************************************/
bool CPtrQueueMT::RemoveHead(QUEUEELEMENT& ptrObj, int nTimeout)
{
	if (false == m_bConsumerAllowed)
		return false;
	ptrObj = NULL;

	if (PrimP(m_hSemFullLck, nTimeout) != 0)
		return false;

	if (PrimP(m_hHeadLck, TW_INFINITE) != 0)
	{
		PrimV(m_hSemFullLck);
		return false;
	}

	ptrObj = m_ptrQueue[m_nHead];
	m_nHead = (m_nHead + 1) % m_unQueueLen;
	m_nElemCnt--;

	if (PrimV(m_hHeadLck) != 0)
		return false;
	if (PrimV(m_hSemEmptyLck) != 0)
		return false;

	return true;
}


/*****************************************************************************
	方法名: RemoveTail
	参数:   
			[out] ptrObj	---- 待取得的指针
			[in]  nTimeout	---- 操作超时时长
	返回值:
			true   ---- 成功
			false  ---- 失败

	说明:   本方法实现从队列尾部取得一个指针，多线程安全

*****************************************************************************/
bool CPtrQueueMT::RemoveTail(QUEUEELEMENT& ptrObj, int nTimeout)
{
	if (false == m_bConsumerAllowed)
		return false;
	ptrObj = NULL;

	if (PrimP(m_hSemFullLck, nTimeout) != 0)
		return false;

	if (PrimP(m_hTailLck, TW_INFINITE) != 0)
	{
		PrimV(m_hSemFullLck);
		return false;
	}

	//回滚尾指针
	m_nTail = (m_nTail - 1 + m_unQueueLen) % m_unQueueLen;

	if (m_nCursor == m_nTail)
		m_nCursor = -1;

	//读出一个指针
	ptrObj = m_ptrQueue[m_nTail];
	m_nElemCnt--;

	if (PrimV(m_hTailLck) != 0)
		return false;
	if (PrimV(m_hSemEmptyLck) != 0)
		return false;

	return true;
}

/*****************************************************************************
	方法名: GotoHead
	参数:   
	返回值:
			true   ---- 成功
			false  ---- 失败

	说明:   本方法实现将m_nCursor移动到m_nHead位置，多线程安全

*****************************************************************************/
void CPtrQueueMT::GotoHead(void)
{
	m_nCursor = m_nHead;
}
/*****************************************************************************
	方法名: PeerNext
	参数:   
			[out] ptrObj	---- 待取得的指针
	返回值:
			true   ---- 成功
			false  ---- 失败

	说明:   本方法实现从m_nCursor位置上返回一个指针，不保证线程安全。
			实际使用时需要加 try...catch...finally

*****************************************************************************/
bool CPtrQueueMT::PeerNext(QUEUEELEMENT& ptrObj)
{
	bool bResult = true;
	ptrObj = NULL;

	try
	{
		if (-1 != m_nCursor)
		{
			if (m_nCursor == m_nTail)
			{
				m_nCursor = -1;
				bResult = false;
			}
			ptrObj = m_ptrQueue[m_nCursor];
			m_nCursor = (m_nCursor + 1) % m_unQueueLen;
		} else
		{
			bResult = false;
		}
	} catch (...)
	{
		bResult = false;
	}
	return bResult;
}

int CPtrQueueMT::SemWait(sem_t* sem, int nTimeout)
{
	assert(nTimeout > 0);

	if (0 == sem_trywait(sem))
		return 0;

	timespec tTime = {0, 10000000};
	timespec tRem = {0};
	int nTimeSlips = (nTimeout / 10) - 1;
	int mTimeLeft = (nTimeout % 10) *  1000000;
	for (int i = 0; i < nTimeSlips; i++)
	{
		if (0 == sem_trywait(sem))
			return 0;
		else
			nanosleep(&tTime, &tRem);
	}
	if (0 == sem_trywait(sem))
		return 0;
	else
	{
		if (mTimeLeft)
		{
			tTime.tv_nsec = mTimeLeft;
			nanosleep(&tTime, &tRem);
			if (0 == sem_trywait(sem))
				return 0;
		}
	}
	return -1;
}
