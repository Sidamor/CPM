#ifndef __MSG_CTL_BASE__
#define __MSG_CTL_BASE__
#include <string>

#include "Shared.h"
#include "MsgMgrBase.h"
#include "ThreadPool.h"
#include "MemMgmt.h"

const int FUNC_TAB_SIZE = 16;

//#define MM_IF_ALLOC()   MM_ALLOC(m_info.enRcver.ulMemHdl)
//#define MM_IF_FREE(p)   MM_FREE(p)

//接口的状态
typedef enum{
	CTL_STA_UP = 0,
	CTL_STA_DOWN,
	CTL_STA_RECONN,
	CTL_STA_LOGON_FAIL,
	CTL_STA_MAX
}CTLStatus;

typedef bool (*CMDFUNC)(void *_this, char *);

typedef struct 
{
	DWORD code;
	CMDFUNC func;
}_FUNCTIONITEM;



//控制的基类 
class CMsgCtlBase 
{
  public:
	//constructor
	CMsgCtlBase();
	//destructor
	virtual ~CMsgCtlBase(void);
	virtual bool Initialize(TKU32 unInputMask,const char *szCtlName);

  protected:
	int AddFunc(DWORD code, CMDFUNC func);
  private:
    _FUNCTIONITEM funcMap[FUNC_TAB_SIZE];
    int funcMapCount;
    TKU32 m_unHandle;
	
	static void *pThrdCtl(void *);
	void MsgCtlFunc(void);

	pthread_t m_thread;
	bool m_bInitialized;
};

#endif
