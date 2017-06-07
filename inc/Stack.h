#ifndef _STACK_H_
#define _STACK_H_

//#include <assert.h>
#include <string>
#include <assert.h>
#include <pthread.h>
#include <string.h>
using namespace std;
//非多线程安全的堆栈
class CFIStack
{
public:
	CFIStack(const int size) : m_nTop(0), m_nSize(size){
		assert(size > 0);
		m_Stack = new size_t[m_nSize];
		assert(m_Stack != NULL);
	}

	virtual ~CFIStack(void) {
		if(NULL != m_Stack)
		{
			delete[] m_Stack;
			m_Stack = NULL;
		}
	}
	
	bool Push(size_t i) {
		
		if(m_nTop < m_nSize)
		{
			m_Stack[m_nTop++] = i;
			return true;
		}
		else
			return false;
	}
	
	bool Pop(size_t& i) {
		
		if(m_nTop > 0)
		{
			i = m_Stack[--m_nTop];
			return true;
		}
		else
			return false;
	}

	int GetCount(void) { return m_nTop;}
protected:
	size_t* m_Stack;
	int m_nTop;
	int m_nSize;
};

//多线程安全的堆栈
class CFIStackMT: public CFIStack
{
public:
	CFIStackMT(const int size) : CFIStack(size) {
			memset(&m_tmLock, 0, sizeof(pthread_mutex_t));
			pthread_mutex_init(&m_tmLock, NULL);
	}
	
	virtual ~CFIStackMT(void) {
			pthread_mutex_destroy(&m_tmLock);
	}

	bool Push(size_t i) {
			pthread_mutex_lock(&m_tmLock);
		bool bRet = CFIStack::Push(i);
			pthread_mutex_unlock(&m_tmLock);
		return bRet;
	}
	
	bool Pop(size_t& i) {
			pthread_mutex_lock(&m_tmLock);
		bool bRet = CFIStack::Pop(i);
			pthread_mutex_unlock(&m_tmLock);
		return bRet;
	}
private:
		pthread_mutex_t 	m_tmLock;				// 操作临界区
};

#endif
