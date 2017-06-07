#include "Init.h"
CInit g_clsInit;
int main()
{	 
	if(!g_clsInit.Initialize())
	{
		return 0;
	}
	
	while(1)
	{
		sleep(1);
	}
	
	return 0;
}
