#ifndef _SYS_MSG_MGR_
#define _SYS_MSG_MGR_

#include <iostream>
#include <assert.h>
#include <stdio.h>
#include "TkMacros.h"
#include "Shared.h"
#include "Stack.h"
#include "QueueMgr.h"

extern FreeMemEvent g_QEMHandler;

//消息管理的状态类型
typedef enum{
	MMR_OK = 0,
	MMR_VOLUME_LIMIT,
	MMR_TUNNEL_EXIST,
	MMR_TUNNEL_NOT_EXIST,
	MMR_GEN_ERR,
	MMR_PEER_EOQ,
	MMR_TUNNEL_MAX
}MMRType;
//消息管理基类
class CMsgMgrBase{
public:
	//constructor，确定消息队列个数
	CMsgMgrBase();
	//destructor，销毁本对象实例
	virtual ~CMsgMgrBase(void);
	//初始化，申请内存。使用实例前必须成功调用本方法
	virtual bool Initialize(int nQueueCnt);
	//增加/设置一个消息通道。
	//unMask= 源模块号<<16 | 目的模块号。模块号定义在Share.h中的SysModType
	//unQueueLen指定了消息通道的长度(可以容纳的消息个数)
//	MMRType SetTunnel(TKU32 unMask, TKU32 unQueueLen);
	//申请消息通道，nTID为返回的消息通道号
	MMRType AllocTunnel(TKU32 unMask, TKU32 unQueueLen, 
							size_t& nTID, FreeMemEvent hdl = g_QEMHandler);
	//释放消息通道，nTID为消息通道号
	MMRType FreeTunnel(int nTID);
	//绑定一个指定的消息通道，获得通道号unHandle。
	MMRType Attach(TKU32 unMask, TKU32& unHandle);
	//发送一个消息到指定的消息通道(放入消息队列的尾部)，
	//qeMsg为消息指针。
	//nTimeout为等待时间，缺省为永远等待，不等待设0，时间单位1/1000秒。
	MMRType SendMsg(TKU32 unHandle, QUEUEELEMENT qeMsg, int nTimeout = TW_INFINITE);
	//发送一个消息到指定的消息通道(放入消息队列的首部)，
	MMRType InsertMsg(TKU32 unHandle, QUEUEELEMENT qeMsg, int nTimeout = TW_INFINITE);
	//从指定消息通道中获得一个新的消息(取消息队列首部的消息)
	MMRType GetFirstMsg(TKU32 unHandle, QUEUEELEMENT &qeMsg, int nTimeout = TW_INFINITE);
	//从指定消息通道中获得一个新的消息(取消息队列尾部的消息)
	MMRType GetLastMsg(TKU32 unHandle, QUEUEELEMENT& qeMsg, int nTimeout = TW_INFINITE);
	//使消息通道的遍历游标回到首部，准备开始遍历
	MMRType GotoHead(TKU32 unHandle);
	//查看消息通道中下一个消息，GotoHead后首次调用
	//将得到第一个消息。线程不安全，请谨慎调用。
	//调用时请使用try...catch保护。
	MMRType PeerNext(TKU32 unHandle, QUEUEELEMENT& qeMsg);
	
	MMRType ConsumerAllowed(TKU32 unHandle, bool isallow);

private:
	int m_nMaxQueueCnt;					//最大消息队列个数
	TKU32* m_pIdxArray;					//索引列表
	CPtrQueueMT* m_pQueueArray;			//消息队列列表
	bool *m_StateArray;					//状态数组
	bool m_Initialized;					//模块是否初始化
	CFIStack* m_pQIDStack;				//通道号堆栈
	pthread_mutex_t m_mtxTnlLock;		//通道操作临界区
};

extern CMsgMgrBase  g_MsgMgr;

#endif
