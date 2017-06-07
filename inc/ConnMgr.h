#ifndef _CONN_MGR_
#define _CONN_MGR_

#include "Shared.h"
#include "ClsTcp.h"
#include "NetIfBase.h"


typedef enum{
	CLT_CONN = 0,
	SVR_CONN
}ConnType;

class ClsConnMgr
{
public:
	ClsConnMgr(void);
	
	virtual ~ClsConnMgr(void);
//	void SetID(int nId) {m_nId = nId;}
//	int GetID(void) {return m_nId;}
	//设置接口模块参数信息
	virtual void SetModIfInfo(ModIfInfo& info) {m_info = info;}
	//获得接口模块参数信息
	const ModIfInfo& GetModIfInfo(void) {return m_info;}
	//LinkDown事件处理方法
	virtual void OnLinkDown(CNetIfBase* ifBase){}
	//返回连接类型
	virtual ConnType GetConnType(void) {return CLT_CONN;}
protected:
	ModIfInfo m_info;
//	int m_nId;
};

//客户端连接管理器
class ClsCltConnMgr: public ClsConnMgr
{
public:
	ClsCltConnMgr(void): ClsConnMgr() {};
	virtual ~ClsCltConnMgr(void);
	//设置接口模块参数信息
	virtual void SetModIfInfo(ModIfInfo& info);
	//依据m_info创建一个到特定主机的连接，主机地址和端口在m_info指定的ini文件中获取
	bool CreateConn(void);
	//关闭ID号为nSocket的连接，释放对应的NetIfBase资源
	bool DropConn(int nSocket);
	//NetIfBase连接断开事件处理方法
	void OnLinkDown(CNetIfBase* ifBase) {}
	//返回连接类型
	virtual ConnType GetConnType(void) {return CLT_CONN;}
};

class ClsSvrConnMgr: public ClsConnMgr, ClsTCPSvrSocket
{
public:
	ClsSvrConnMgr(int nMaxLinks = MAX_LINK_NMB): ClsConnMgr(), ClsTCPSvrSocket(nMaxLinks) {};
	virtual ~ClsSvrConnMgr(void);
	char * GetSvrLocal(char * buf,int bufsize){ GetLocal(buf, bufsize);return buf; }
	bool ListenAt(ClsInetAddress& ia,  tpport_t port);
	//设置接口模块参数信息
	virtual void SetModIfInfo(ModIfInfo& info, CNetIfBase* pNetIf=NULL);
	//NetIfBase连接断开事件处理方法
	void OnLinkDown(CNetIfBase* ifBase);
	//返回连接类型
	virtual ConnType GetConnType(void) {return SVR_CONN;}
	
protected:
	//Server socket连接请求事件处理方法
	CNetIfBase* m_pNetIf;
	virtual bool OnAccept(int nSocket);
};

#endif



