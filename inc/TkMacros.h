
#ifndef __TKMAXCRO_HDR__
#define __TKMAXCRO_HDR__

#include <stdio.h>
#include <string.h>

#ifdef DEBUG
#define DBGOUT(a)   cout << a;
#define TRC(a)		printf("%s\n", #a);
#define DPRINT(a)   printf a;
#define DBG(a)		printf a;
#define DBGLINE		printf("FILE=%s, LINE=%d\n", __FILE__, __LINE__);
#else
#define DBGOUT(a)
#define TRC(a)
#define DPRINT(a)   
#define DBG(a)	
#define DBGLINE		
#endif

#ifdef TRACE_STACK
#define TRACE(a)	printf("TRACE> %s\n", #a);
#else
#define TRACE(a)
#endif


#ifdef TRACE_STACK1
#define TRC_STACK1(a)	printf("%s\n", #a);
#else
#define TRC_STACK1(a)	
#endif

#ifdef TRACE_STACK2
#define TRC_STACK2(a)	printf("%s\n", #a);
#else
#define TRC_STACK2(a)
#endif

// 
// define the basic type for Took Kit
//

typedef unsigned int 				TKU32;
typedef unsigned short 				TKU16;
typedef unsigned char 				TKU8;

typedef int 						TKS32;
typedef short 						TKS16;
typedef char 						TKS8;


#define UNUSED_PARAM(a)				(a=a)

// print error information to screen
#define PRINT_ERROR(a)			printf("File(%s) Line(%d)\t", __FILE__, __LINE__); printf a;
#define PRINT_INFO(a)				DBG(a);

#endif



