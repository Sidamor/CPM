#ifndef _CONN_CONTAINER_
#define _CONN_CONTAINER_
#include <utility>
#include <tr1/unordered_map>

//#include <tr1/unordered_set>
#include "ConnMgr.h"
using namespace std;
class ConnContainer{
public:
	ConnContainer();
	~ConnContainer();
	bool Initialize(int nConnCnt); //nConnCnt的2倍容量初始化哈希表
	//判断是否已经初始化
	bool IsInitialized(void) {return m_bIsInitialized;}
	//加入一个连接管理器和NetIfBase的关联
	bool AddConn(ClsConnMgr* mgr, CNetIfBase* ifBase);
	
	//删除一个连接，nIfId为NetIfBase内部保存的接口号
	//该方法将是释放接口号为nIfId的NetIfBase实例
	//bDelNow决定是否立即释放
	bool RemoveConn(int nIfId, bool bDelIfNow = true);

	bool RemoveConn(CNetIfBase* ifBase, bool bDelIfNow = true)
	{
		return (ifBase) ? RemoveConn(ifBase->GetIfId(), bDelIfNow) : false;
	}
	
	//待释放NetIfBase的扫描线程，
	//调用RemoveConn时标明bDelNow为false的NetIfBase实例在本线程中被释放
	static void* pThrdScanner(void *);
	
	//处理NetIfBase的LinkDown事件的Callback函数
	static void OnLinkDown(int nIfId);

private:
	//CFIStack* m_IdStack;
	CFIStackMT* m_IdStack;

	//连接池哈希表节点结构
	typedef struct {
		int nKey;			//nKey = nIfId，实际是SocketId
		ClsConnMgr* connMgr;
		CNetIfBase* ifBase;
	}ConnInfo;
	
	
	struct HASHER
	{
		size_t operator()(const int &s) const
		{
			return s;
		}
	};

	struct EQUALFUNC
	{
		bool operator()(const int &a, const int &b) const
		{
			return a==b;
		}
	};

	typedef tr1::unordered_map<int, ConnInfo, HASHER, EQUALFUNC> _CONN_HASH;

	_CONN_HASH* m_pConnHash;

	pthread_mutex_t m_mtxHashLock;

	pthread_t m_thread;
	bool m_bIsInitialized;
};

extern ConnContainer g_ConnContainer;

#endif

