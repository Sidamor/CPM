#ifndef _DDNSClient_H_
#define _DDNSClient_H_
#include "constDef.h"
#include "IniFile.h"
#include "Md5.h"
#include "ClsUdp.h"

class DDNSClient
{
public:
	DDNSClient();
 ~DDNSClient();
	bool init();
		
  void MD5(unsigned char *szBuf, unsigned char *src, unsigned int len);
	void Time2Chars(time_t* t_time, char* pTime);
	private:
	
	void SendRunThrdRun();
private:
	int  m_Status;
	char m_Url[60];
	char m_Pwd[10];
	char m_SvrIp[20];
	int  m_SvrPort;
	
	int m_Sock;
	ClsSocket* m_pSock;
	sockaddr *m_peer;
	int m_nPeerSize;
};
#endif
