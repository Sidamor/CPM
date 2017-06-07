#ifndef __AUDIO_PLAY_H__
#define __AUDIO_PLAY_H__

#include "ThreadPool.h"

#pragma pack(1)
enum
{	
	STA_AUDIO_INIT,
	STA_AUDIO_PALY,
	STA_AUDIO_STOP,
	STA_AUDIO_PAUSE
};

typedef struct _FMT_WAV_HEAD
{
	char resv1[22];
	unsigned short channel;
	unsigned int samplerate;
	char resv2[6];
	unsigned short bits;
}FMT_WAV_HEAD;

#pragma pack()
//接口的基类 
class CAudioPlay {
public:
	CAudioPlay();
	//destructor
	virtual ~CAudioPlay(void);
	//使用前必须调用的方法，判断是否成功
	bool Initialize(char* devPath);
	bool Terminate();

	bool StartPlay(char* audiopath);
	bool StopPlay();
protected:

	static void *PlayThrd(void *);
	bool Play(void);

private:
	int SetAudioPara(int nSampleRate,int nChannels,int fmt);

private:
	class Lock
	{
	private:
		pthread_mutex_t	 m_lock;
	public:
		Lock() { pthread_mutex_init(&m_lock, NULL); }
		bool lock() { return pthread_mutex_lock(&m_lock)==0; }
		bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
	}CTblLock;

private:
	char m_strDevPath[128];			//声卡设备文件名
	int m_hdlWavDev;				//声卡设备句柄
	int m_PlayStatus;				//当前声卡状态,STA_WAV_DEVICE

	char m_strAudioPath[128];		//音频文件路径
	FILE* hdlAudioFile;				//音频文件句柄	
	int m_LenFrame;					//音频帧长度

	pthread_t m_hdlPlayThrd;			//播放线程句柄
};

#endif
