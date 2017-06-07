#include "Init.h"
#include "DDNSClient.h"

DDNSClient g_clsDDNSClient;


bool CInit::Initialize(void)
{	
	int m_Status = 0;
	get_ini_int("Config.ini", "DDNS", "DDnsStatus", -1, &m_Status);
	if(0 == m_Status)
	{
		return false;
	}
	
	if(!g_clsDDNSClient.init()) 
	{
		return false;
	}
	
	return true;
}
