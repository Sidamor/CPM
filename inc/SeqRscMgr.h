#ifndef _SEQ_RSC_MGR_
#define _SEQ_RSC_MGR_

#include <pthread.h>
typedef int SEQNO;

typedef  int (* GetValEvent)(const int nKey);
typedef  int (* GetKeyEvent)(const SEQNO nVal);

class CSeqRscMgr{
public:
	CSeqRscMgr(void);
	virtual ~CSeqRscMgr(void);
	//使用前必须成功调用的方法，
	//nNmb为可重复利用的序列号个数，
	//hGetVal为Key映射为实际序列号的函数指针,
	//hGetKey为实际序列号映射为Key的函数指针
	bool Initialize(int nNmb, GetValEvent hGetVal, GetKeyEvent hGetKey);
	//判断是否成功初始化
	bool IsInitialized(void) {return m_bInitialized;}
	
	//获得实际序列号，序列号从nVal返回
	bool Alloc(int& nVal);
	//回收序列号，
	bool Free(const SEQNO val);
	
	//获得序列号的总容量
	int GetTotalVol(void) {return m_nRscCnt;}
	//获得已使用的序列号个数
	int GetUsedVol(void) {return m_nUsedCnt;}
	//判断一个key的序列号是否正在使用中
	bool IsInUse(int& nKey);
	//获得所有正在使用的序列号，buff为信息存放的缓存，size为缓存的字节数
	char* sPrintRscInUse(char* buff, int size);
private:
	int* m_KeyArray;
	SEQNO* m_SeqArray;
	bool* m_RscState;
	int m_nUsedCnt;
	int m_nRscCnt;
	bool m_bInitialized;
	pthread_mutex_t     m_tmLock;				// 操作临界区
	GetValEvent OnGetVal;
	GetKeyEvent OnGetKey;
};

extern CSeqRscMgr g_SeqRscMgr;

#endif  

