#include "ClsUdp.h"

ClsUDPSocket::ClsUDPSocket(void) :ClsSocket(AF_INET, SOCK_DGRAM, 0)
{
	TRACE(ClsUDPSocket::ClsUDPSocket);
	memset(&m_peer, 0, sizeof(m_peer));
	m_peer.sin_family = AF_INET;
}


ClsUDPSocket::ClsUDPSocket(const ClsInetAddress &ia, tpport_t port) :
ClsSocket(AF_INET, SOCK_DGRAM, 0)
{
	TRACE(ClsUDPSocket::ClsUDPSocket);
	SetServerSocket(ia, port);
}

bool ClsUDPSocket::SetServerSocket(const ClsInetAddress &ia, tpport_t port)
{
	TRACE(ClsUDPSocket::SetServerSocket);
	// if socket is not created, return
	// if the socket is already binded, return
	if (m_socket < 0 || SOCKET_BOUND == m_state)
		return false;

	memset(&m_local, 0, sizeof(m_local));
	m_local.sin_family = AF_INET;
	m_local.sin_addr = ia.GetAddress();
	m_local.sin_port = htons(port);

#ifdef  SO_REUSEADDR
	int opt = 1;
	setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
			   (TKU32)sizeof(opt));
#endif

	if (bind(m_socket, (struct sockaddr *)&m_local, sizeof(m_local)))
	{
		EndSocket();
		return false;
	}
	m_state = SOCKET_BOUND;

	return true;
}


void ClsUDPSocket::SetPeer(TKU32 ipAddr, tpport_t port)
{
	TRACE(ClsUDPSocket::SetPeer);

	m_peer.sin_family = AF_INET;
	m_peer.sin_addr.s_addr = htonl(ipAddr);
	m_peer.sin_port = htons(port);
}

void ClsUDPSocket::SetPeer(const ClsInetAddress &ia, tpport_t port)
{
	TRACE(ClsUDPSocket::SetPeer);

	memset(&m_peer, 0, sizeof(m_peer));
	m_peer.sin_family = AF_INET;
	m_peer.sin_addr = ia.GetAddress();
	m_peer.sin_port = htons(port);
}

ClsInetAddress ClsUDPSocket::GetPeer(tpport_t *port) const
{
	TRACE(ClsUDPSocket::GetPeer);

	char buf;
	int len = sizeof(m_peer);
	int rtn = ::recvfrom(m_socket, &buf, 1, MSG_PEEK, 
						 (struct sockaddr *)&m_peer, (socklen_t *)len);

	if (rtn < 1)
	{
		if (port)
			*port = 0;

		memset((void*) &m_peer, 0, sizeof(m_peer));
	} else
	{
		if (port)
			*port = ntohs(m_peer.sin_port);
	}
	return ClsInetAddress(m_peer.sin_addr);
}  

char * ClsUDPSocket::GetPeer(char *buf, int sz)
{
	TRACE(ClsUDPSocket::GetPeer);

	snprintf(buf, sz, "addr %d.%d.%d.%d, port %d",
		(TKU32)(m_peer.sin_addr.s_addr &0xff),
		(TKU32)(m_peer.sin_addr.s_addr >> 8)&0xff,
		(TKU32)(m_peer.sin_addr.s_addr >> 16)&0xff,
		(TKU32)(m_peer.sin_addr.s_addr >> 24)&0xff,			 
		ntohs(m_peer.sin_port));
	buf[sz - 1] = 0;

	return buf;
}


char * ClsUDPSocket::GetLocal(char *buf, int sz)
{
	TRACE(ClsUDPSocket::GetLocal);

	snprintf(buf, sz, "addr %d.%d.%d.%d, port %d",
		(TKU32)(m_local.sin_addr.s_addr &0xff),
		(TKU32)(m_local.sin_addr.s_addr >> 8)&0xff,
		(TKU32)(m_local.sin_addr.s_addr >> 16)&0xff,
		(TKU32)(m_local.sin_addr.s_addr >> 24)&0xff,
		ntohs(m_local.sin_port));
	buf[sz - 1] = 0;

	return buf;
}
