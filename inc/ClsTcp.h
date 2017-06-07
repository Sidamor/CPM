#ifndef _TCP_HDR_
#define _TCP_HDR_

#include "ThreadPool.h"
#include "TkMacros.h"   // define macro: TRACE
#include "ClsSocket.h"

#define DEF_LINK_NMB	128
#define MAX_LINK_NMB	256

//TCP socket基础类
class ClsTCPSocket :public ClsSocket {
public:
	ClsTCPSocket(int nSocket = -1);
    ~ClsTCPSocket()
    {
		TRACE(ClsTCPSocket::~ClsTCPSocket);
	}

	//获得本地监听端口信息，用于打印显示
	char * GetLocal(char *buf, int sz);

	//返回本地监听端口地址和端口
	void GetLocal(sockaddr_in *sock) { memcpy(sock, &m_local, sizeof(sockaddr_in)); }
	//返回本地监听端口地址和端口
	const sockaddr_in *GetLocal() { return (const sockaddr_in *)&m_local; }
	//设置socket操作超时
	inline void SetTimeout(int nMSec = DEF_TIMEOUT)
	{
		m_nTimeout = nMSec;
	}

protected:
    struct sockaddr_in 	m_local;
	TKU32 m_nTimeout;
	enum{
		DEF_TIMEOUT = 500
	};
};

//TCP客户端socket
class ClsTCPClientSocket: public ClsTCPSocket
{
public:
	ClsTCPClientSocket();
	//使用一个已经连接的socket初始化
	ClsTCPClientSocket(int nClientId);
	//连接到监听在ia和port上的主机软件
	//bReconnect将断开现有连接，并重新连接
	int Connect(const ClsInetAddress &ia, tpport_t port, bool bReconnect = false);
	//判断是否已经连接上
	inline bool IsConnected(void) {return (SOCKET_CONNECTED == m_state);}
	// send 
	int Send(void *buf, size_t len);

	int Recv(char *buf, int len);

public:	
	//设置对方监听地址和端口
    void SetPeer(const ClsInetAddress &ia, tpport_t port);

	// NAME: SetPeer()
	// SUMMARY:
	//     It is used to set the peer address with ip address and port
	//
	//设置对方监听地址和端口
    void SetPeer(TKU32 ipAddr, tpport_t port);

	//获得客户端端口信息，用于打印显示
	char * GetPeer(char *buf, int sz);
	//返回客户端端口对应的对方地址和端口
	inline void GetPeer(TKU32 *ip, u_short *port)
	{
		*ip = ntohl(m_peer.sin_addr.s_addr);
		*port = ntohs(m_peer.sin_port);
	}
	//返回客户端端口对应的对方地址和端口
	const sockaddr_in *GetPeer() { return (const sockaddr_in *)&m_peer; }
	//返回客户端端口对应的对方地址和端口
	void GetPeer(sockaddr_in *sock) { memcpy(sock, &m_peer, sizeof(sockaddr_in)); }
private:
    struct sockaddr_in 	m_peer;
    bool m_bIsConnected;
	fd_set m_stReadFDs;
	fd_set m_stWriteFDs;
};



//Tcp Server Socket
typedef enum {
	AM_CALLBACK,	//Accept缺省采用Callback模式，调用OnAccept函数指针
	AM_INHERIT,		//ClsTcpSvrSocket的继承类建议采用继承模式
	AM_MAX
}ConnAcceptMode;

//Accept事件回调函数指针类型
typedef void (* AcceptEvent)(int nSocket);

//TCP服务户端监听用socket
class ClsTCPSvrSocket :public ClsTCPSocket
{
public:
	//构造函数，nMaxLinks表示最大可接受连接数, 
	//MAX_LINK_NMB现定义为256，DEF_LINK_NMB为128
	ClsTCPSvrSocket(int nMaxLinks = DEF_LINK_NMB);
	// create a TCP Server socket which binds to a specific interface
	ClsTCPSvrSocket(const ClsInetAddress &ia, tpport_t port, int nMaxLinks = DEF_LINK_NMB);

   virtual ~ClsTCPSvrSocket()
    {
		TRACE(~ClsTCPSvrSocket);
		Final();
	}
	// SetServerSocket()
	// set the socket server address,
	// return 	false			server address already set or socket not created.
	//			true			it is set successfully
	//设置本地监听地址和端口
	bool SetServerSocket(const ClsInetAddress &ia, tpport_t port);

	//返回Accept事件发生时采用的事件响应模式
	ConnAcceptMode GetAcceptMode(void) {return m_enmMode;}
	/********************************************************************/
	/*设置Accept事件发生时采用的事件响应模式							*/
	/*AM_CALLBACK模式将调用CallbackOnAccept回调函数指针					*/
	/*AM_INHERIT模式将调用OnAccept虚函数，								*/
	/*继承类应改写该方法以便获得事件的处理权							*/
	/********************************************************************/
	void SetAcceptMode(ConnAcceptMode &mode) { m_enmMode = mode;}

	//获得AM_CALLBACK模式下CallbackOnAccept回调函数指针
	AcceptEvent GetAcceptHandler(void) {return 	CallbackOnAccept;}
	//设置AM_CALLBACK模式下CallbackOnAccept回调函数指针
	//返回最近使用的CallbackOnAccept回调函数指针
	AcceptEvent SetAcceptHandler(AcceptEvent newHandler) 
	{
		AcceptEvent oldHandler = CallbackOnAccept;
		CallbackOnAccept = newHandler;
		return 	oldHandler;
	}

	//判断Server socket当前是否接受客户端连接请求
	bool GetAcceptAllowed(void) {return m_AcceptAllowed;}
	//设置Sever socket当前是否接受客户端连接请求
	void SetAcceptAllowed(bool acceptAllowed) {m_AcceptAllowed = acceptAllowed;}

	//在指定地址和端口上监听，返回0表示成功
	int Listen(void);
	//启动Server socket连接处理线程，并采用mode指定的模式处理Accept事件
	bool Run(ConnAcceptMode mode = AM_CALLBACK);
	//减少连接数，当一个该Server socket建立的连接被关闭时应调用该方法
	void ReduceConn(void) {m_nUsedLinkNmb--;}
	//显式释放资源，包括线程
	void Final(void);
	//Server socket连接处理线程
	static void* AcceptThrd(void*);
	
protected:
	virtual void init(int nMaxLinks);
	virtual int Accept(void);
	virtual bool OnAccept(int nSocket) {TRACE(ClsTCPSvrSocket::OnAccept); return false;}
private:
	int m_nMaxLinkNmb;
	int m_nUsedLinkNmb;
	int m_nIOCtrlFlag;
	fd_set m_stReadFDs;
	int m_lastLinkPos;
	bool m_AcceptAllowed;
	ConnAcceptMode m_enmMode;
	AcceptEvent CallbackOnAccept;
	bool m_bIsInitialized;
	pthread_t m_thread;
};


#endif
