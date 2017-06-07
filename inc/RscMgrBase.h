#ifndef _RSC_MGR_BASE_
#define _RSC_MGR_BASE_
#include <pthread.h>

<template class T> 
struct _RscNode{
	T val;
	bool bInUse;
};

class CRscBase{
public:
	TKU32 GetKey(void) {return m_unKey;}
	void SetKey(TKU32 unKey) {m_unKey = unKey;}
	bool IsInUse(void) {return m_bInUse;}
private:
	TKU32 m_unKey;
	bool m_bInUse;
};


typedef _RscNode<T>  RscNode;

<template class T, TKU32 nmb = 1024> 
class RscMgrBase{
public:
	RscMgrBase(bool bWaitValue = false);
	virtual ~RscMgrBase(void);
	int AddRsc(T& val);
	int AddRsc(T* val);
	bool Alloc(T& val);
	const bool Alloc(T*& val);
	bool Free(T& val);
	bool Free(T* val);
	TKU32 GetTotalVol(void) {return m_nRscCnt;}
	TKU32 GetValidVol(void) {return m_nValidCnt;}
	TKU32 GetUsedVol(void) {return m_nUsedCnt;}
	bool IsInitialized(void) {return m_bInitialized;}
private:
	TKU32 m_nKeyArray[nmb];
	RscNode<T> m_pstRscArray[nmb];
	TKU32 m_nRscCnt;
	TKU32 m_nUsedCnt;
	TKU32 m_nValidCnt;
	bool m_bInitialized;
	pthread_mutex_t     m_tmLock;				// ²Ù×÷ÁÙ½çÇø
};

#endif

