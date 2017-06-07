#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>
#include <signal.h>
#include <stdlib.h>

int g_iHandle = -1;


int main(int argc, char **argv)
{
	int i = 0;
	int iTime = -1;

	//打开设备
	g_iHandle = open ("/dev/watchdog", O_RDWR);//O_RDWR
	if (0 > g_iHandle)
	{
		printf("open failed \n");
		return 0;
	}
	printf("iHandle = %d\n", g_iHandle);


	//开启看门狗 
	ioctl(g_iHandle, WDIOC_SETOPTIONS, WDIOS_ENABLECARD);


	iTime = 60;
	printf("Set watchdog timer %d seconds\n", iTime);
	ioctl(g_iHandle, WDIOC_SETTIMEOUT, &iTime);
	
	ioctl(g_iHandle, WDIOC_GETTIMEOUT, &iTime);
	printf("Time = %d\n", iTime);


	while (0 < g_iHandle)
	{
		printf("%d\n", ++ i);
		write(g_iHandle, "1", 1);//喂狗
		sleep(1);
	}

	return 0;
}
