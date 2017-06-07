#include "DDNSClient.h"

DDNSClient::DDNSClient()
{
	
}
 
DDNSClient::~DDNSClient()
{
	
}

bool DDNSClient::init()
{
	m_Status = 0;
	memset(m_Url, 0, sizeof(m_Url));
	memset(m_Pwd, 0, sizeof(m_Pwd));
	memset(m_SvrIp, 0, sizeof(m_SvrIp));
	m_SvrPort = 0;
	
	get_ini_int("Config.ini", "DDNS", "DDnsStatus", -1, &m_Status);
	get_ini_string("Config.ini", "DDNS", "DDnsUrl", "=", m_Url, sizeof(m_Url));
	get_ini_string("Config.ini", "DDNS", "DDnsPwd", "=", m_Pwd, sizeof(m_Pwd));
	get_ini_string("Config.ini", "DDNS", "DDnsSvrIp", "=", m_SvrIp, sizeof(m_SvrIp));
	get_ini_int("Config.ini", "DDNS", "DDnsSvrPort", -1, &m_SvrPort);
	
	printf("m_SvrPort:%d\n", m_SvrPort);
	
	SendRunThrdRun();
	
	return true;
}

//发送线程
void DDNSClient::SendRunThrdRun()
{
	while(1)
	{
		if(1 == m_Status)
		{
			char md5_input[256] = {0};
			unsigned char md5_output[16] = {0};
			char md5_hex[33] = {0};		
			
			//域名
			int i=56;
			sprintf(m_Url, "%-56s", m_Url);
			memcpy(md5_input, m_Url, 56);	

			//时间
			time_t nowtime;
			time(&nowtime);		
			char szTime[15] = {0};
			Time2Chars(&nowtime, szTime);
			memcpy(md5_input+i, szTime, 14);
			i += 14;

			//密码
			memcpy(md5_input+i, m_Pwd, strlen(m_Pwd));
			i += strlen(m_Pwd);

			//运算生成md5
			MD5(md5_output, (unsigned char *)md5_input, 76);
			char* ptrMd5Hex = md5_hex;
			for(int i=0; i<16; i++)
			{
				sprintf(ptrMd5Hex, "%02x", md5_output[i]);
				ptrMd5Hex += 2;
			}	
			
			//发送包
			char sendData[1024] = {0};
			sprintf(sendData, "%-56s%-14s%-32s", m_Url, szTime, md5_hex);
			printf("sendData:%s\n", sendData);
					
			ClsUDPSocket* pSock = NULL;
			if(m_pSock) 
			{
				delete m_pSock;
				m_pSock = NULL;
			}
			m_pSock = pSock = new ClsUDPSocket;
			if(!m_pSock) 
			{
				continue;
			}
				
			pSock->SetPeer(m_SvrIp, m_SvrPort);
				
			m_peer = (sockaddr*) pSock->GetPeer();
			m_nPeerSize = sizeof(sockaddr);
			
			m_Sock = socket(AF_INET, SOCK_DGRAM, 0);
			if(0 == connect(m_Sock, (struct sockaddr *)m_peer, m_nPeerSize))
			{
				send(m_Sock, sendData, strlen(sendData), 0);
			}	
		}	
		
		close(m_Sock);
		
		if(m_pSock)
		{
			delete m_pSock;
			m_pSock = NULL;
		}
		sleep(5);
	}
}

//MD5运算
void DDNSClient::MD5(unsigned char *szBuf, unsigned char *src, unsigned int len)
{
	MD5_CTX context;

	MD5Init(&context);
	MD5Update(&context, src, len);
	MD5Final(szBuf, &context);
}

//时间转换
void DDNSClient::Time2Chars(time_t* t_time, char* pTime)
{
	int year=0,mon=0,day=0,hour=0,min=0,sec=0;
    struct tm *m_tm;
    m_tm = gmtime(t_time);
    year = m_tm->tm_year+1900;
    mon = m_tm->tm_mon+1;
    day = m_tm->tm_mday;
    hour = m_tm->tm_hour;
    min = m_tm->tm_min;
    sec = m_tm->tm_sec;
    sprintf(pTime, "%04d%02d%02d%02d%02d%02d", year, mon, day, hour, min, sec);
}
