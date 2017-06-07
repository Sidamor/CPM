#ifndef __MEM_MGMT__
#define __MEM_MGMT__
#include <pthread.h>

#define MAX_MUCB_NUM	64

//	接口函数的宏定义，支持内存DEBUG模式
#ifdef MM_DEBUG
	#define MM_ALLOC(ulMH)			g_MemMgr.DbgAlloc(ulMH)
	#define MM_FREE(ptrMB)			g_MemMgr.DbgFree(ptrMB)
	#define MM_CHECK(ptrMB)			g_MemMgr.DbgCheckT(ptrMB)

	#define MM_ALLOC_H(ulMH)		g_MemMgr.DbgAlloc_H(ulMH)
	#define MM_FREE_H(ptrMB, ulMH)	g_MemMgr.DbgFree_H(ptrMB)
	#define MM_FREE_G(ptrMB, ulMH)	g_MemMgr.DbgFree_G(ptrMB)
	#define MM_CHECK_H(ptrMB, ulMH)	g_MemMgr.DbgCheck_H(ptrMB)
#else
	#define MM_ALLOC(ulMH)			g_MemMgr.Alloc(ulMH)
	#define MM_FREE(ptrMB)			g_MemMgr.Free(ptrMB)
	#define MM_CHECK(ptrMB)			g_MemMgr.Check(ptrMB)

	#define MM_ALLOC_H(ulMH)		g_MemMgr.Alloc_H(ulMH)
	#define MM_FREE_H(ptrMB, ulMH)	g_MemMgr.Free_H(ptrMB, ulMH)
	#define MM_FREE_G(PtrMB)		g_MemMgr.Free_G(ptrMB)
	#define MM_CHECK_H(ptrMB, ulMH) g_MemMgr.Check_H(ptrMB, ulMH)
#endif


// 用户内存管理控制块结构
typedef struct{
	unsigned long			ulUserMCBHdl;		// 用户内存控制块句柄
	unsigned long			ulOrgElemSize;		// 用户请求的每条信息的尺寸
	unsigned long			ulActElemSize;		// 实际提供的每元素尺寸（系统调整为可以被sizeof(unsigned long)整除）
	unsigned long			ulElemCount;		// 元素数量
	unsigned long			ulMemSize;			// 内存区尺寸 = ulActElemSize * ulElemCount
}MMUserCtrlBlkStruct;

// 定义内存指针类型
typedef void* MMMemPtr;

// 内存管理控制块结构
typedef struct{
	MMUserCtrlBlkStruct stUCM;				// 用户内存管理控制块
	char*				ptrMem;				// 内存区
	MMMemPtr*			ptrMemPtr;			// 内存地址索引
	unsigned long		ulMemCnt;			// 可以申请的剩余内存块数量
	pthread_mutex_t     csLock;				// 操作临界区
}MMCtrlBlkStruct;


// 内存操作结果类型
typedef enum{
	MMO_OK = 1,					// 内存管理操作成功
	MMO_FAILED = 0				// 内存管理操作失败
}MMOREnum;

class CMemMgr{
public:
	CMemMgr(void);
	~CMemMgr(void);
	//	初始化内存管理模块
	MMOREnum	Initialize(unsigned long ulMaxRsvBlk = MAX_MUCB_NUM);

//内存块管理基本模式，内存块释放时必须给出对应的申请句柄
public:		
	// 用户注册保留使用的内存
	MMOREnum	RegRsvMem_H(MMUserCtrlBlkStruct* pstMUCB);
	//	申请某种内存,内存块头部包含句柄
	void*		Alloc_H(unsigned long ulMCBHandle);
	//	指定句柄释放内存，速度快(关键是解决重复释放问题)
	MMOREnum	Free_H(void*& ptrMemBlock, unsigned long ulMCBHandle);
	//	不指定句柄释放内存
	MMOREnum	Free_G(void*& ptrMemBlock);

//内存块管理高级模式，内存块释放时无需给出对应的申请句柄
public:		
	// 用户注册保留使用的内存
		MMOREnum	RegRsvMem(MMUserCtrlBlkStruct* pstMUCB);
	//	申请某种内存,内存块头部包含句柄
		void*		Alloc(unsigned long ulMCBHandle);
	//	释放内存,内存块头部包含句柄
		MMOREnum	Free(void*& ptrMemBlock);

#if defined(MM_DEBUG)

// =====================DEBUG 版本函数==============================//

	//	DEBUG版：申请某种内存
	void*		DbgAlloc_H(unsigned long ulMCBHandle, 
		const char* lpszFileName = __FILE__, int nLine = __LINE__);
	//	DEBUG版：指定类型释放内存，速度快(关键是解决重复释放问题)
	MMOREnum	DbgFree_H(void*& ptrMemBlock, unsigned long ulMCBHandle, 
		const char* lpszFileName = __FILE__, int nLine = __LINE__);
	//	DEBUG版：释放内存(关键是解决重复释放问题)
	MMOREnum	DbgFree_G(void*& ptrMemBlock, 
		const char* lpszFileName = __FILE__, int nLine = __LINE__);
	//	DEBUG版：检查当前使用的内存指针是否合法
	void		DbgCheck_H(void* ptrMemBlock, unsigned long ulMCBHandle, 
		const char* lpszFileName = __FILE__, int nLine = __LINE__);


	//	DEBUG版：申请某种内存
	void*		DbgAlloc(unsigned long ulMCBHandle, 
		const char* lpszFileName = __FILE__, int nLine = __LINE__);
	//	DEBUG版：释放内存
	MMOREnum	DbgFree(void*& ptrMemBlock,  
		const char* lpszFileName = __FILE__, int nLine = __LINE__);
	//	检查当前使用的内存指针是否合法
	void		DbgCheck(void* ptrMemBlock, 
		const char* lpszFileName = __FILE__, int nLine = __LINE__);

#endif
	
private:
	// 定义和初始化内存控制块数组
	MMCtrlBlkStruct *m_pstMMCBArray;
	
    // 最大内存控制块数量
    	unsigned long m_ulMaxMUCBNum;
    
	// 内存控制块使用计数器
	unsigned long m_ulMMCBUseCnt;
	
	// 内存注册临界区
	pthread_mutex_t m_mtxRegLock;
};

extern CMemMgr g_MemMgr;

#endif
