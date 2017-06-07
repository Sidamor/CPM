#ifndef _EM_API_H_
#define _EM_API_H_

#ifdef __cplusplus
    extern "C" {
#endif

typedef int HANDLE;

/*********************************************
*	IoControl Functions
**********************************************/
int EmClose(int hDevice);
int EmOpen(int *pHandle); 
int EmOpenWithName(int *pHandle, const char *device_name);

int EmGetDriverVersion(int hDevice, char *vData, int data_size);

int EmApiSetGDO0(const int hDevice, const int c_iData);
int EmApiGetGDO0(const int hDevice);//
int EmApiGetGDO2(const int hDevice);

int EmApiReset(const int hDevice);

int EmApiSetPA4(const int hDevice, const int c_iFlag);
int EmApiSetPB28(const int hDevice, const int c_iFlag);

int EmApiSetCts1(const int hDevice, const int c_iFlag);
int EmApiSetCts2(const int hDevice, const int c_iFlag);

int EmApiSend595Data(const int hDevice, const int c_iData);
int EmApiSetBuzz(const int hDevice, const int c_iData);

//面板前灯相关
int EmApiSetDebug(const int hDevice, const int c_iFlag);//SYS
int EmApiGsmPowerOn(const int hDevice);//GSM
int EmApiGsmPowerOff(const int hDevice);//GSM
int EmApiSetRF(const int hDevice, const int c_iFlag);//433
int EmApiSetPLA(const int hDevice, const int c_iFlag);//PLA
int EmApiSetALM(const int hDevice, const int c_iFlag);//ALM


#ifdef __cplusplus
    }
#endif

#define EMClose(a)				EmClose(a)
#define EMOpen(a)				EmOpen(a)
#define EMOpenWithName(a,b)		EmOpenWithName(a,b)
#define EMGetDriverVersion(hDevice, vData, data_size)	EmGetDriverVersion( hDevice, vData, data_size)

#define EMApiSetGDO0(hDevice, c_iData)	EmApiSetGDO0(hDevice, c_iData)
#define EMApiGetGDO0(hDevice)	EmApiGetGDO0(hDevice)//
#define EMApiGetGDO2		EmApiGetGDO2(hDevice)

#define EMApiReset(hDevice)			EmApiReset(hDevice)

#define EMApiSetPA4(hDevice, c_iFlag)			EmApiSetPA4(hDevice, c_iFlag)
#define EMApiSetPB28(hDevice, c_iFlag)		EmApiSetPB28(hDevice, c_iFlag)

#define EMApiSetCts1(hDevice, c_iFlag)		EmApiSetCts1(hDevice, c_iFlag)
#define EMApiSetCts2(hDevice, c_iFlag)		EmApiSetCts2(hDevice, c_iFlag)

#define EMApiSend595Data(hDevice, c_iData)	EmApiSend595Data(hDevice, c_iData)
#define EMApiSetBuzz(hDevice, c_iData)		EmApiSetBuzz(hDevice, c_iData)

//面板前灯相关
#define EMApiSetDebug(hDevice, c_iFlag)		EmApiSetDebug(hDevice, c_iFlag)//SYS
#define EMApiGsmPowerOn(hDevice)		EmApiGsmPowerOn(hDevice)//GSM
#define EMApiGsmPowerOff(hDevice)	EmApiGsmPowerOff(hDevice)//GSM
#define EMApiSetRF(hDevice, c_iFlag)			EmApiSetRF(hDevice, c_iFlag)//433
#define EMApiSetPLA(hDevice, c_iFlag)			EmApiSetPLA(hDevice, c_iFlag)//PLA
#define EMApiSetALM(hDevice, c_iFlag)			EmApiSetALM( hDevice, c_iFlag)//ALM

#endif


