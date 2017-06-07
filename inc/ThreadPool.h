// ThreadPool.h the declaration of the thread management class

#ifndef __THREAD__POOL__0198237412038LASDFJASD__HH__
#define __THREAD__POOL__0198237412038LASDFJASD__HH__

#include <pthread.h>
#include <vector>
#include <string>
#include <iostream>
#include <functional>
using namespace std;
typedef void* (* ThreadFunc)(void* arg);
//线程创建结构信息
struct ThreadCreateInfo
{
    ThreadFunc func;
    void* arg;
    pthread_attr_t* attr;
    ThreadCreateInfo() {}
    ThreadCreateInfo(ThreadFunc func_in) : func(func_in),
       arg(0), attr(0) {}
};
//线程信息
struct ThreadInfo
{
    bool system;
    pthread_t tid;
    pid_t pid;
    ThreadCreateInfo createInfo;
    std::string name;
    ThreadInfo() {}
    ThreadInfo(pthread_t id) : tid(id) {}
};

inline bool operator==(const ThreadInfo& info, const ThreadInfo& infoa)
{
    return info.tid == infoa.tid;
}
//线程池
class ThreadPool
{
public:
    enum { THREAD_MAX = 512 };
    static ThreadPool* Instance();
    bool StartMonitor();
    bool CreateThread(std::string name, ThreadFunc func, bool system = false,
    	    void* arg = 0, pthread_attr_t* attr = 0);
    void CancelThread(pthread_t tid);
    void GetThreadsInfo(std::vector<ThreadInfo>* info);
    bool SetThreadMode(pthread_t tid, bool IsSystem);
private:
    pthread_mutex_t mutex_;
    std::vector<ThreadInfo> threads_;
    bool monitorStarted_;
    
    ThreadPool();
    static void* StaticThreadFunction(void* arg);
    void ThreadFunction();
    static void* StaticMonitorThread(void* arg);
    void MonitorThread();
};

#endif
