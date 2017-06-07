#ifndef __PTR_QUEUE_MT__1
#define __PTR_QUEUE_MT__1

#include <semaphore.h>
#include <string>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include "TkMacros.h"
using namespace std;

typedef void* QUEUEELEMENT;
#define MAX_QUEUE_LEN	1024
#define TW_INFINITE		-1

typedef  int (* FreeMemEvent)(QUEUEELEMENT ptrObj);

class CPtrQueueMT 
{
// operations
public:
	CPtrQueueMT();
	virtual ~CPtrQueueMT(void);
	/*****************************************************************************
	方法名: Initialize
	参数:  [in] unQueueLen	---- 队列长度
		   [in]	hRelMem表示响应队列清除消息内容事件的callback函数指针
	返回值:	true		成功
				false		失败

	说明:	本方法在CPtrQueueMT类实例被创建后调用，
			完成初始化队列控制信号灯，队列头尾指针和队列内存的任务
			
	*****************************************************************************/
	bool Initialize(TKU32 unQueueLen = MAX_QUEUE_LEN, FreeMemEvent hRelMem = NULL);
	
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
	bool AddTail(QUEUEELEMENT ptrObj, int nTimeout = TW_INFINITE);

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
	bool RemoveHead(QUEUEELEMENT& ptrObj, int nTimeout = TW_INFINITE);

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
	bool AddHead(QUEUEELEMENT ptrObj, int nTimeout = TW_INFINITE);

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
	bool RemoveTail(QUEUEELEMENT& ptrObj, int nTimeout = TW_INFINITE);

	/*****************************************************************************
		方法名: GotoHead
		参数:   
		返回值:
				true   ---- 成功
				false  ---- 失败
	
		说明:   本方法实现将m_nCursor移动到m_nHead位置，多线程安全
	
	*****************************************************************************/
	void GotoHead(void);
	
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
	bool PeerNext(QUEUEELEMENT& ptrObj);
	//判断消息队列是否初始化
	bool IsInitialized(bool) {return m_bInitialized;}
	
	/*****************************************************************************
	方法名: Finalize
	参数:	无
	返回值: 无
	
	说明:	清理函数，本方法在CPtrQueueMT类实例被删除时被析构函数调用，
			完成释放队列控制信号灯句柄和队列内存的任务
	*****************************************************************************/
	void Finalize(void);
	//阻塞消息发送者
	void BlockProducer(void) {m_bProducerAllowed = false;}
	//取消对消息发送者阻塞
	void UnBlockProducer(void) {m_bProducerAllowed = true;}
	//阻塞消息接收者
	void BlockConsumer(void) {m_bConsumerAllowed = false;}
	//取消对消息接收者的阻塞
	void UnBlockConsumer(void) {m_bConsumerAllowed = true;}
// Implementation
	bool m_bProducerAllowed;
	bool m_bConsumerAllowed;
protected:
	sem_t m_hSemEmptyLck;				// 队列空间用完信号灯
	sem_t m_hSemFullLck;				// 队列满信号灯
	sem_t m_hHeadLck;					// 头指针锁
	sem_t m_hTailLck;					// 尾指针锁

	bool m_bInitialized;
	TKU32 m_unQueueLen;					// 消息队列长度

	QUEUEELEMENT* m_ptrQueue;			// 消息队列
	
	int m_nHead;						// 头部下标
	int m_nTail;						// 尾部下标
	int m_nCursor;						// 遍历游标
	int m_nElemCnt;

	FreeMemEvent OnRelMem;
protected:
	int PrimP(sem_t& hSem, int nTimeout);
	int PrimV(sem_t& hSem);
	int SemWait(sem_t* sem, int nTimeout);
	void Init();
};

#endif
