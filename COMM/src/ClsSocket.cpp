#include "ClsSocket.h"

void ClsInetAddress::SPrint(char *buf, int len)
{
	TRACE(ClsInetAddress::SPrint);
	snprintf(buf, len, "%d.%d.%d.%d", 
			 (TKU32)(m_ipaddr.s_addr >> 24)&0xff,
			 (TKU32)(m_ipaddr.s_addr >> 16)&0xff,
			 (TKU32)(m_ipaddr.s_addr >> 8)&0xff,
			 (TKU32)(m_ipaddr.s_addr &0xff));
	buf[len - 1] = 0;
}

bool ClsInetAddress::Set(const char *hostname)
{
	TRACE(ClsInetAddress::Set); 

	m_valid = true;
//	 if hostname is not provided, any address is returned
	if (!hostname || (hostname[0] == '*'))
	{
		m_ipaddr.s_addr = htonl(INADDR_ANY);
		return m_valid;
	}

	struct    hostent *ph;
	// check whether it is dotted decimal format
	m_ipaddr.s_addr = inet_addr(hostname);
	if (m_ipaddr.s_addr == (TKU32)-1)
	{
		// if not, resolve the host address
		if ((ph = gethostbyname(hostname)) == NULL)
		{
			perror("SrvrOpenSocket().gethostbyname()");
			m_valid = false;
		} else
		{
			// use the first address
			struct sockaddr_in sockaddr_in;
			memcpy((char *)&sockaddr_in.sin_addr,(char *)ph->h_addr_list[0], ph->h_length);
			m_ipaddr = sockaddr_in.sin_addr;
		}
	}
	return m_valid;
}

ClsSocket::ClsSocket(int domain, int type, int protocol, int nSocket)
{
	TRACE(ClsSocket::ClsSocket);    

	m_state = SOCKET_INITIAL;
	m_socket = nSocket;

	if (-1 == m_socket)
		m_socket = socket(domain, type, protocol);

	if (m_socket >= 0)
	{
		m_state = SOCKET_AVAILABLE;
	}
}



bool ClsSocket::IsPending(int timeout, bool retOnInt)
{
	TRACE(ClsSocket:IsPending);

	struct pollfd pfd;
	int status = 0;

	pfd.fd = m_socket;
	pfd.events = POLLIN;

	while (status < 1)
	{

		if (timeout < 0)
			status = poll(&pfd, 1, -1);
		else
			status = poll(&pfd, 1, timeout);

		if (status < 1)
		{
			// don't stop polling because of a simple
			// signal :)
			if (status == -1 && errno == EINTR && retOnInt == false)
				continue;

			return false;
		}
	}

	if (pfd.revents & pfd.events)
	{
		return true;
	}
	return false;
}

void ClsSocket::EndSocket(void)
{
	TRACE(ClsSocket::EndSocket);
/*
	if(m_state == SOCKET_STREAM)
	{
		m_state = SOCKET_INITIAL;
		if(m_socket > -1)
		{
			close(m_socket);
			m_socket = -1;
		}
		return;
	}
*/
	m_state = SOCKET_INITIAL;
	if (m_socket < 0)
		return;
	close(m_socket);
	m_socket = -1;
}

