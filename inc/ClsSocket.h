#ifndef _SOCKET_HDR_
#define _SOCKET_HDR_

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <poll.h> 
#include <errno.h>

#include "TkMacros.h"   // define macro: TRACE
using namespace std;
typedef int tpport_t;

enum SocketState
{
    SOCKET_INITIAL,
    SOCKET_AVAILABLE,
    SOCKET_BOUND,
    SOCKET_CONNECTED,
    SOCKET_RECONNECTED,
    SOCKET_CONNECTING
};

//IP地址类型，用于统一IP V4地址的表达
class ClsInetAddress 
{
    struct in_addr    m_ipaddr;
	bool			  m_valid;
public:
	ClsInetAddress(void)
	{
		m_valid = false;	
        m_ipaddr.s_addr = 0;
	}
	//采用UINT初始化地址类，高位是地址的开始部分
	ClsInetAddress(TKU32 ipAddr) { Set(ipAddr); }
	//采用in_addr类型初始化地址类
	ClsInetAddress(struct in_addr addr) { Set(addr); }
	//采用地址字符串初始化地址类
	ClsInetAddress(const char *address) { Set(address); }
	//采用UINT设置地址，高位是地址的开始部分
	inline void Set(TKU32 ipAddr)
	{
		m_valid = true;	
        m_ipaddr.s_addr = htonl(ipAddr);
	}
	//采用in_addr类型设置地址
	inline void Set(struct in_addr addr)
	{
		m_valid = true;
		m_ipaddr = addr;
	}
	//采用地址字符串设置地址
	bool Set(const char *address);
	//判断地址是否有效
	inline bool operator!() const{return !m_valid;}
	
	//获得in_addr类型的IP地址
	struct in_addr GetAddress(void) const {return m_ipaddr;}
	//获得UINT类型的IP地址
	inline TKU32 GetIpAddress(void){ return ntohl(m_ipaddr.s_addr);}
	//打印地址信息，buf是信息存放的内存，len指buf的字节数
	void SPrint(char *buf, int len);
};


//Socket基础类
class ClsSocket 
{
protected:
	SocketState     m_state;
	int 			m_socket;
public:
	/********************************************************************/
	/*构造函数，domain一般为AF_INET										*/
	/*type一般为SOCK_DGRAM和SOCK_STREAM，分别对应UDP和TCP				*/
	/*protocol一般填0，具体参考系统API: socket							*/
	/*nSocket一般为-1，大于零表示使用一个已有的连接初始化Socket			*/
	/********************************************************************/
	ClsSocket(int domain, int type, int protocol = 0, int nSocket = -1);
    ~ClsSocket()
    {
		TRACE(~ClsSocket);
		EndSocket();
	}
	
	/********************************************************************/
	/*判断Socket连接上是否有新数据或请求								*/
	/*timeout指等待时间，单位毫秒，-1表示等待直到有新的数据或请求		*/
	/*retOnInt表示是否在系统中断到来时返回调用							*/
	/********************************************************************/
	bool IsPending(int timeout = -1, bool retOnInt = false);
	//判断socket是否已经绑定
	inline bool IsSocketBound(void) const { return SOCKET_BOUND == m_state; }
	//返回上一次socket错误
	inline int GetLastSockErr(void) const { return errno; }
	//关闭socket连接
	void EndSocket(void);
	//返回socket ID
	int GetSockId(void) {return m_socket;}
	//判断socket ID是否无效，未初始化
    inline bool operator!() const {return m_socket < 0;};
    //设定socket阻塞/非阻塞模式，缺省为阻塞模式
	inline int SetCompletion(bool block = true)
	{
		int fflags = fcntl(m_socket, F_GETFL);
		switch( block )
		{
			case false:
				fflags |= O_NONBLOCK;
				break;
			case true:
				fflags &=~ O_NONBLOCK;
				break;
		}
		return fcntl(m_socket, F_SETFL, fflags);
	}
	//设置协议栈接收缓存大小
	inline int SetRcvBuf(int size) 
		{ return setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(size));}
	//设置协议栈发送缓存大小
	inline int SetSndBuf(int size)
		{ return setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (const char*)&size, sizeof(size));}
	//获得协议栈接收缓存大小
	inline int GetRcvBuf(int *size) 
		{ int len = sizeof(*size); return getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char *)size, (socklen_t *) len);}
	//获得协议栈接收缓存大小
	inline int GetSndBuf(int *size)
		{ int len = sizeof(*size); return getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char *)size, (socklen_t *) len);}
	//设置socket是否重用，Server软件监听口一般应设置为true
	inline int SetSockOptReuse(bool reuse)
	{ 
		int optval = reuse?1:0;
    	return setsockopt( m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval));
	}
};
#endif
