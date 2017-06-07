#ifndef _constDef_H_
#define _constDef_H_

#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
	#include <time.h>
	#pragma comment(lib,"Wsock32.lib")
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <sys/types.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <string>
	#include <poll.h>
  #include <pthread.h>
	#include <time.h>
#endif

#define  SleepTime 5//√Î
#define  DWORD unsigned long

void StrRightFillSpace(char* str, int len);
#endif
