#ifndef _ROUTE_MGR_
#define _ROUTE_MGR_

#include <string>

#include <tr1/unordered_map>
#include "ClsSocket.h"
using namespace std;
#define DEF_ROUTE_NMB	512

class CRouteMgr
{
public:
	CRouteMgr();
	~CRouteMgr();
	//使用前必须成功调用的方法，
	//nmb表示路由的数量
	bool Initialize(const int nmb = DEF_ROUTE_NMB);
	//判断是否成功初始化
	bool IsInitialized(void) {return m_bInitialized;}
	//添加消息通道号到Ip地址的路由映射
	//TID表示通道号
	//ia和port表示地址和端口
	bool AddRouteI2A(const int TID, const ClsInetAddress &ia, short int port);
	//作用同上，ia的高位为地址的起始点
	bool AddRouteI2A(const int TID, const TKU32 ia, short int port);
	//删除消息通道号到Ip地址的路由映射
	bool DelRouteI2A(const int TID);
	//添加Ip地址到消息通道号的路由映射
	//TID表示通道号
	//ia和port表示地址和端口
	bool AddRouteA2I(const ClsInetAddress &ia, short int port, const int TID);
	//作用同上，ia的高位为地址的起始点
	bool AddRouteA2I(const TKU32 ia, short int port, const int TID);
	//删除Ip地址到消息通道号的路由映射
	bool DelRouteA2I(const ClsInetAddress &ia, short int port);
	//作用同上，ia的高位为地址的起始点
	bool DelRouteA2I(const TKU32 ia, short int port);
	//查找通道号对应的ip地址
	// if address returned with struct memory zeroed, means lookup failed.
	sockaddr_in GetAddrByTID(const int TID);
	//查找Ip地址对应的通道号
	// return -1, means lookup failed.
	int GetTIDByAddr(const ClsInetAddress &ia, short int port);
	//作用同上，ia的高位为地址的起始点
	int GetTIDByAddr(const TKU32 ia, short int port);

private:	
	typedef struct {
		int nTID;			//nKey = nIfId，实际是SocketId
		unsigned long addr;
		tpport_t port;
	}RouteNode;
	
// Hash TID to Address	
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

	typedef tr1::unordered_map<int, RouteNode, HASHER, EQUALFUNC> _I2A_HASH;
	typedef tr1::unordered_map<int, RouteNode, HASHER, EQUALFUNC> _A2I_HASH;

	_I2A_HASH* m_phtI2A;	//TID to Address
	_A2I_HASH* m_phtA2I;	//Address to TID

	bool m_bInitialized;

	pthread_mutex_t m_mtxI2ALock;
	pthread_mutex_t m_mtxA2ILock;

};

extern CRouteMgr g_RouteMgr;


#endif


