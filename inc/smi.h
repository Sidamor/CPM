#ifndef _SMI_DEAL_H
#define _SMI_DEAL_H

#ifdef __cplusplus
    extern "C" {
#endif

int ApiSmiMacRead (const HANDLE hDevice, const int PhyId, const int RegNum);
int ApiSmiMacWrite (const HANDLE hDevice, const int PhyId, const int RegNum, const int WriteData);


//把边上网口的关闭      0x1d    0x8
int PowerDownNetPortOne(const HANDLE hDevice);

//把边上的网口开启      0x1d    0x0
int PowerUpNetPortOne(const HANDLE hDevice);

//把里边网口的关闭      0x2d    0x8
int PowerDownNetPortTwo(const HANDLE hDevice);

//把里边的网口开启      0x2d    0x0
int PowerUpNetPortTwo(const HANDLE hDevice);




//放狗
int ReleaseWatchDog(void);

//设置看门狗超时时间
int SetWatchDogTimer(int iSeconds);

//喂狗
int FeedWatchDog(void);



#ifdef __cplusplus
    }
#endif


#endif

