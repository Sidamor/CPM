#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "AudioPlay.h"
#include <linux/soundcard.h>
#include "Shared.h"


CAudioPlay::CAudioPlay()
{
	memset(m_strDevPath, 0, sizeof(m_strDevPath));
	m_hdlWavDev = -1;	//音频设备

	memset(m_strAudioPath, 0, sizeof(m_strAudioPath));

	m_PlayStatus = STA_AUDIO_STOP;
	m_hdlPlayThrd = 0xffff;
	m_LenFrame = 0;
}   

//destructor
CAudioPlay::~CAudioPlay(void)
{
	Terminate();
}
bool CAudioPlay::Terminate()
{
	if(0xFFFF != m_hdlPlayThrd)
	{
		ThreadPool::Instance()->SetThreadMode(m_hdlPlayThrd, false);
		if (pthread_cancel(m_hdlPlayThrd)!=0)
			;//DBGLINE;
	}

	if (NULL != hdlAudioFile)
	{
		fclose(hdlAudioFile);
		hdlAudioFile = NULL;
	}

	if (-1 != m_hdlWavDev)
	{
		close(m_hdlWavDev);
		m_hdlWavDev = -1;
	}
	return true;
}

bool CAudioPlay::Initialize(char* devName)
{
	strcpy(m_strDevPath, devName);

	if (-1 == (m_hdlWavDev = open(m_strDevPath, O_WRONLY)))
	{
		printf("Open Audio device[%s] failed.\n", devName);
		return false;
	}

	char buf[256] = {0};
	sprintf(buf, "%s %s", "AudioPlay", "Play");
	if (!ThreadPool::Instance()->CreateThread(buf, &CAudioPlay::PlayThrd, true,this))
	{
		return false;
	}
	return true;
}

//播放线程
void* CAudioPlay::PlayThrd(void *para)
{
	CAudioPlay* object = (CAudioPlay*)para;
	object->Play();
	return NULL;
}
//线程执行体
bool CAudioPlay::Play(void)
{
	unsigned char audiobuf[512*1024] = {0};
	int bytesread = 0;
	m_hdlPlayThrd = pthread_self();
	printf("CAudioPlay::Play Start .... Pid[%d]\n", getpid());
	while (1)
	{
		pthread_testcancel();

		if(0 > m_hdlWavDev)//打开音频设备
		{	
			m_hdlWavDev = open(m_strDevPath, O_WRONLY);
			sleep(1);
			continue;
		}
		CTblLock.lock();
		//DBG(("Play 1111..."));
		switch(m_PlayStatus)
		{
		case STA_AUDIO_INIT:
			{
				DBG(("STA_AUDIO_INIT 1111..."));
				if (NULL != hdlAudioFile)
				{
					DBG((" close audio file...111\n"));
					fclose(hdlAudioFile);
					hdlAudioFile = NULL;
				}
				DBG(("STA_AUDIO_INIT 2222..."));
				//路径不合法
				if (strlen(m_strAudioPath) <=0)
				{
					DBG((" m_strAudioPath[%s] inValid..\n", m_strAudioPath));
					sleep(1);
					break;
				}
				
				DBG(("STA_AUDIO_INIT 3333..."));
				//打开音频文件
				if(NULL == (hdlAudioFile = fopen(m_strAudioPath, "rb")))
					break;
				
				DBG(("STA_AUDIO_INIT 4444..."));
				//获取音频信息
				FMT_WAV_HEAD wavHead;
				int readsize = fread((unsigned char*)&wavHead, 1, sizeof(FMT_WAV_HEAD), hdlAudioFile);
				if (sizeof(FMT_WAV_HEAD) != readsize)
				{
					DBG(("WAV FMT error size[%d]", readsize));
					AddMsg((unsigned char*)&wavHead, readsize);
					fclose(hdlAudioFile);
					hdlAudioFile = NULL;
					break;
				}
				
				DBG(("STA_AUDIO_INIT 5555..."));
				fseek(hdlAudioFile, 0, 0);
				DBG(("rate[%d] channel[%d] bits[%d]\n",wavHead.samplerate, wavHead.channel, wavHead.bits));

				DBG(("STA_AUDIO_INIT 66666..."));
				//配置声卡模式
				int setret = -1;
				if (0 != (setret = SetAudioPara(wavHead.samplerate, wavHead.channel, wavHead.bits)))
				{
					DBG(("SetAudioPara Failed errorcode[%d]\n", setret));
					fclose(hdlAudioFile);
					hdlAudioFile = NULL;
					m_PlayStatus = STA_AUDIO_STOP;
					sleep(1);
					break;
				}
				
				DBG(("STA_AUDIO_INIT 7777..."));
				m_PlayStatus = STA_AUDIO_PALY;
			}
			break;
		case STA_AUDIO_PALY:
			{

				DBG(("Play 2222..."));
				if (NULL == hdlAudioFile)
				{
					m_PlayStatus = STA_AUDIO_INIT;
					break;
				}

				DBG(("Play 3333..."));
				//到文件尾，则从头播放
				if (feof(hdlAudioFile))
				{
					DBG(("Fseek ...\n"));
					fseek(hdlAudioFile, 0, 0);
				}
				if ((unsigned int)m_LenFrame > sizeof(audiobuf))
				{
					DBG(("Wav file is not normal.\n"));
					fclose(hdlAudioFile);
					hdlAudioFile = NULL;
					m_PlayStatus = STA_AUDIO_STOP;
					sleep(1);
					break;;
				}

				DBG(("Play 4444... \n"));
				int totalsize = 0;
				bool loop = true;
				while (loop && totalsize < m_LenFrame && !feof(hdlAudioFile))
				{
					if (0 < (bytesread = fread(audiobuf, 1, m_LenFrame, hdlAudioFile)))
					{
						DBG(("Audio Play..333 \n"));
						int pos = 0;
						totalsize += bytesread;
						while(pos < bytesread)
						{
							int wlen = write(m_hdlWavDev, audiobuf + pos, bytesread - pos);
							if (wlen <= 0)
							{
								DBG(("write audiodev failed\n"));
								loop = false;
								break;
							}
							//		AddMsg(audiobuf + pos, wlen);
							pos += wlen;
						}
					}
					else 
					{
						perror("fread failed:");
						fclose(hdlAudioFile);
						hdlAudioFile = NULL;
						m_PlayStatus = STA_AUDIO_INIT;
						break;
					}
					DBG(("Audio Play..555[%d] readLen[%d] totalsize[%d]\n", m_LenFrame, bytesread, totalsize));
				}

				DBG(("Play 5555..."));

			}
			break;
		case STA_AUDIO_STOP:
			if (NULL != hdlAudioFile)
			{
				DBG(("StartPlay close...111\n"));
				fclose(hdlAudioFile);
				hdlAudioFile = NULL;
				memset(m_strAudioPath, 0, sizeof(m_strAudioPath));
			}
			sleep(5);//5秒
			break;
		case STA_AUDIO_PAUSE:
			break;
		default :
			break;
		}

	//	DBG(("Play 6666..."));
		usleep(200*1000);//0.2秒
		CTblLock.unlock();

	//	DBG(("Play 7777..."));

	}
	DBG(("Exit Thrd ..................................\n"));
	return true;	
}

bool CAudioPlay::StartPlay(char* audiopath)
{
	bool ret = false;
	CTblLock.lock();

	m_PlayStatus = STA_AUDIO_INIT;
	if (NULL != hdlAudioFile)
	{
		fclose(hdlAudioFile);
		hdlAudioFile = NULL;
	}

	memset(m_strAudioPath, 0, sizeof(m_strAudioPath));
	strcpy(m_strAudioPath, audiopath);
	DBG(("CAudioPlay::StartPlay(%s)\n", audiopath));
	ret = true;
	CTblLock.unlock();
	return ret;

}

bool CAudioPlay::StopPlay()
{
	CTblLock.lock();
	m_PlayStatus = STA_AUDIO_STOP;
	DBG(("Audio stop play..\n"));
	CTblLock.unlock();
	return true;

}

int CAudioPlay::SetAudioPara(int nSampleRate,int nChannels,int fmt)
{
	int status,arg;
	arg = nSampleRate;
	status = ioctl(m_hdlWavDev,SOUND_PCM_WRITE_RATE,&arg); /*set samplerate*/
	if(status < 0)
	{
		close(m_hdlWavDev);
		m_hdlWavDev = -1;
		return -1;
	} 
	if(arg != nSampleRate)
	{
		close(m_hdlWavDev);
		m_hdlWavDev = -1;
		return -2;
	} 
	arg = nChannels;  /*set channels*/   
	status = ioctl(m_hdlWavDev, SOUND_PCM_WRITE_CHANNELS, &arg); 
	if(status < 0)
	{
		close(m_hdlWavDev);
		m_hdlWavDev = -1;
		return -3;
	} 
	if( arg != nChannels)
	{
		close(m_hdlWavDev);
		m_hdlWavDev = -1;
		return -4;
	}
	arg = fmt; /*set bit fmt*/
	status = ioctl(m_hdlWavDev, SOUND_PCM_WRITE_BITS, &arg); 
	if(status < 0)
	{
		close(m_hdlWavDev);
		m_hdlWavDev = -1;
		return -5;
	} 
	if(arg != fmt)
	{
		close(m_hdlWavDev);
		m_hdlWavDev = -1;
		return -6;
	}
	m_LenFrame = 3*nChannels*nSampleRate*fmt/8;
	/*到此设置好了DSP的各个参数*/  
	return 0; 
}
