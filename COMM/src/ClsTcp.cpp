#include "ClsTcp.h"
#include "Shared.h"

/*********************************************************************************
TCP Socket Base definition
*********************************************************************************/

ClsTCPSocket::ClsTCPSocket(int nSocket) :ClsSocket(AF_INET,SOCK_STREAM, 0, nSocket)
{
	TRACE(ClsTCPSocket::ClsTCPSocket);

	m_nTimeout = DEF_TIMEOUT;
	memset(&m_local, 0, sizeof(m_local));
	m_local.sin_family = AF_INET;
}

char * ClsTCPSocket::GetLocal(char *buf, int sz)
{
	TRACE(ClsTCPSocket::GetLocal);
	snprintf(buf, sz, "addr %d.%d.%d.%d, port %d\n",
		(TKU32)(m_local.sin_addr.s_addr &0xff),
		(TKU32)(m_local.sin_addr.s_addr >> 8)&0xff,
		(TKU32)(m_local.sin_addr.s_addr >> 16)&0xff,
		(TKU32)(m_local.sin_addr.s_addr >> 24)&0xff,
		ntohs(m_local.sin_port));
	buf[sz - 1] = 0;

	return buf;
}

/*********************************************************************************
TCP Client Socket definition
*********************************************************************************/

ClsTCPClientSocket::ClsTCPClientSocket()
{
	TRACE(ClsTCPClientSocket::ClsTCPClientSocket);
}

ClsTCPClientSocket::ClsTCPClientSocket(int nClientId):ClsTCPSocket(nClientId)
{
	TRACE(ClsTCPClientSocket::ClsTCPClientSocket);

	int nLen = sizeof(m_local);
	if ((0 == getsockname(m_socket, (sockaddr*)&m_local, (socklen_t*)&nLen)) &&
		(0 == getpeername(m_socket, (sockaddr*)&m_peer, (socklen_t*)&nLen)))
	{
		m_state = SOCKET_CONNECTED;
	} else
	{
		m_state = SOCKET_INITIAL;
	}
}

int ClsTCPClientSocket::Connect(const ClsInetAddress &ia, tpport_t port, bool bReconnect)
{
	TRACE(ClsTCPClientSocket::Connect);
	if (SOCKET_CONNECTED == m_state)
	{
		if (false == bReconnect)
		{
			return -1;
		} else
		{
			EndSocket();
			SetPeer(ia, port);
			m_socket = socket(m_peer.sin_family, SOCK_STREAM, 0);
			if (m_socket < 0)
			{
				return -1;
			}
		}
	}
	if (SetCompletion(true) == -1)
	{
		return -1;
	}

	SetPeer(ia, port);
	int nRet = connect(m_socket,(const sockaddr *)&m_peer, sizeof(m_peer));
	if (-1 != nRet)
	{
		m_state = SOCKET_CONNECTED;
	} 
	else
	{
		m_state = SOCKET_INITIAL;
	}

	return nRet;
}

int ClsTCPClientSocket::Send(void *buf, size_t len)
{
	TRACE(ClsTCPClientSocket::Send);
	int nRet = 0;
	int nLen = len;
	char Temp[256] = {0};
	sprintf(Temp, "Send[TCPClient] To Socket=%d,DataLen=%zu\n",m_socket,len);
	DataLogSock(FUNC_HEXINFO,Temp,(char *)buf,len);
	if (SOCKET_CONNECTED == m_state)
	{
		while (1)
		{
			nRet = send(m_socket, buf, nLen, 0);
			if (nRet < 0)
			{
				if (EINTR != errno)
				{
					EndSocket();
					return -1;
				} 
			} 
			else
			{
				return nRet;
			}
		}
	} 
	else
	{
		return -1;
	}
}


int ClsTCPClientSocket::Recv(char *buf, int len)
{
	TRACE(ClsTCPClientSocket::Recv);
	char Temp[256] = {0};
	int nRet = 0;
	int nLen = len;
	if (SOCKET_CONNECTED == m_state)
	{
		while (1)
		{
			nRet = recv(m_socket, buf, nLen, 0); 
			if (nRet <= 0)
			{
				if (nRet == 0)
				{
					EndSocket();
					return -1;
				}
				sprintf(Temp, "[TCPClient]Socket=%d,EINTR=%d,ErrNo=%d\n",m_socket,EINTR,errno);
				DataLogSock(FUNC_STRINFO, (char*)"",Temp,strlen(Temp));
			} 
			else
			{
				sprintf(Temp, "Recv[TCPClient] From Socket=%d,DataLen=%d\n",m_socket,nRet);
				DataLogSock(FUNC_HEXINFO,Temp,(char *)buf,nRet);
				return nRet;
			}
		}
	}
	return -1;
}

void ClsTCPClientSocket::SetPeer(TKU32 ipAddr, tpport_t port)
{
	TRACE(ClsTCPClientSocket::SetPeer);

	m_peer.sin_family = AF_INET;
	m_peer.sin_addr.s_addr = htonl(ipAddr);
	m_peer.sin_port = htons(port);
}

void ClsTCPClientSocket::SetPeer(const ClsInetAddress &ia, tpport_t port)
{
	TRACE(ClsTCPClientSocket::SetPeer);

	memset(&m_peer, 0, sizeof(m_peer));
	m_peer.sin_family = AF_INET;
	m_peer.sin_addr = ia.GetAddress();
	m_peer.sin_port = htons(port);
}


char * ClsTCPClientSocket::GetPeer(char *buf, int sz)
{
	TRACE(ClsTCPClientSocket::GetPeer);

	snprintf(buf, sz, "addr %d.%d.%d.%d, port %d\n",
			(TKU32)(m_peer.sin_addr.s_addr &0xff),
			(TKU32)(m_peer.sin_addr.s_addr >> 8)&0xff,
			(TKU32)(m_peer.sin_addr.s_addr >> 16)&0xff,
			(TKU32)(m_peer.sin_addr.s_addr >> 24)&0xff,			 
			 ntohs(m_peer.sin_port));
	buf[sz - 1] = 0;

	return buf;
}


/*********************************************************************************
TCP Server Socket definition
*********************************************************************************/

ClsTCPSvrSocket::ClsTCPSvrSocket(int nMaxLinks)
{
	TRACE(ClsTCPSvrSocket::ClsTCPSvrSocket);

	init(nMaxLinks);
}
// create a TCP Server socket which binds to a specific interface
ClsTCPSvrSocket::ClsTCPSvrSocket(const ClsInetAddress &ia, tpport_t port, int nMaxLinks)
{
	TRACE(ClsTCPSvrSocket::ClsTCPSvrSocket);

	init(nMaxLinks);
	SetServerSocket(ia, port);
}

void ClsTCPSvrSocket::init(int nMaxLinks)
{
	TRACE(ClsTCPSvrSocket::init);

	m_nMaxLinkNmb = nMaxLinks;
	m_nUsedLinkNmb = 0;
	m_nIOCtrlFlag = 0;
	memset(&m_stReadFDs, 0, sizeof(fd_set));
	m_lastLinkPos = 0;
	m_AcceptAllowed = true;
	CallbackOnAccept = NULL;
	m_bIsInitialized = false;
	m_thread = 0xFFFF;
}

bool ClsTCPSvrSocket::SetServerSocket(const ClsInetAddress &ia, tpport_t port)
{
	TRACE(ClsTCPSvrSocket::SetServerSocket);

	if (m_socket < 0 || SOCKET_BOUND == m_state)
		return false;

	memset(&m_local, 0, sizeof(m_local));
	m_local.sin_family = AF_INET;
	m_local.sin_addr = ia.GetAddress();
	m_local.sin_port = htons(port);

#ifdef  SO_REUSEADDR
	int opt = 1;
	setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,(TKU32)sizeof(opt));
#endif

	if (bind(m_socket, (struct sockaddr *)&m_local, sizeof(m_local)))
	{
		EndSocket();
		return false;
	}
	m_state = SOCKET_BOUND; 
	return true;
}

int ClsTCPSvrSocket::Listen(void)
{
	TRACE(ClsTCPSvrSocket::Listen);

	if (listen(m_socket, m_nMaxLinkNmb)==-1)
	{
		close(m_socket);
		return -1;
	}
	if (SetCompletion(false) == -1)
	{
		close(m_socket);
		return -1;
	}
	return 0;
}

bool ClsTCPSvrSocket::Run(ConnAcceptMode mode)
{
	TRACE(ClsTCPSvrSocket::Run);
	assert(m_bIsInitialized == false);
	assert(mode < AM_MAX);

	char buf[256] = {0};
	m_enmMode = mode;
	GetLocal(buf, sizeof(buf));

	if (!ThreadPool::Instance()->CreateThread(buf, ClsTCPSvrSocket::AcceptThrd, true, this))
	{
		TRACE("Accept Thread start failed.\n");
		return false;
	}
	m_bIsInitialized = true;

	return true;
}

int ClsTCPSvrSocket::Accept(void)
{
	TRACE(ClsTCPSvrSocket::Accept);
	assert(m_bIsInitialized == true);

	int nClientId = -1;
	int nSize = sizeof(m_local);
	FD_ZERO(&m_stReadFDs);
	FD_SET(m_socket, &m_stReadFDs);
	if (true == IsPending(m_nTimeout))
	{
		if (FD_ISSET(m_socket, &m_stReadFDs))
		{
			nClientId = accept(m_socket,(sockaddr *)&m_local, (socklen_t*)&nSize);
			char Temp[255] = {0};
			sprintf(Temp, "[TCPClient]Socket=%d,nClientId=%d",m_socket,nClientId);
			DataLogSock(FUNC_STRINFO, (char*)"",Temp,strlen(Temp));
		}
	} else
	{
		nClientId = -1;
	}
	return nClientId;
}

void* ClsTCPSvrSocket::AcceptThrd(void* Ptr)
{
	TRACE(ClsTCPSvrSocket::AcceptThrd);
	ClsTCPSvrSocket *_this = (ClsTCPSvrSocket*) Ptr;
	_this->m_thread = pthread_self();
	usleep(1000*20);
	while (1)
	{
		pthread_testcancel();

		if ((_this->m_AcceptAllowed == true) && (_this->m_nUsedLinkNmb < _this->m_nMaxLinkNmb))
		{
			int nRet = _this->Accept();
			if (nRet >= 0)
			{
				DBG(("SocketId[%d]\n", nRet));
				switch (_this->m_enmMode)
				{
				case AM_CALLBACK:

					if (NULL != _this->CallbackOnAccept)
					{
						try
						{
							_this->CallbackOnAccept(nRet);
							_this->m_nUsedLinkNmb++;
						} catch (...)
						{

							close(nRet);
						}
					} else
					{
						close(nRet);
					}
					break;
				case AM_INHERIT:
					if (_this->OnAccept(nRet))
					{
						_this->m_nUsedLinkNmb++;
					} else
					{
						close(nRet);
					}
					break;
				default:                        
					close(nRet);
					break;
				}
			}
			else
				sleep(1);
		} else
		{
			sleep(1);
		}
	}
}

void ClsTCPSvrSocket::Final(void)
{
	if(m_bIsInitialized)
	{
		if(m_thread != 0xFFFF)
		{
			ThreadPool::Instance()->SetThreadMode(m_thread, false);
			pthread_cancel(m_thread);
		}
		EndSocket();
		m_bIsInitialized = false;
	}
}
