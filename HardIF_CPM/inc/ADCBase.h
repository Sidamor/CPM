#ifndef __ADC_BASE_H__
#define __ADC_BASE_H__

typedef enum _ADC_BAUDRATE
{
	ADC_BAUDRATE_1200 = 1,
	ADC_BAUDRATE_2400,
	ADC_BAUDRATE_4800,
	ADC_BAUDRATE_9600,
	ADC_BAUDRATE_19200,
	ADC_BAUDRATE_38400,
	ADC_BAUDRATE_57600,
	ADC_BAUDRATE_115200
}ADC_BAUDRATE;
class CADCBase {
	
public :
	CADCBase(int mode);
    ~CADCBase();
    bool Initialize(char* devName);
	void Terminate();
	bool SetBaudRate(int serial_index, ADC_BAUDRATE baudrate);
    bool SetSerialOpt(int serial_index, const int c_iStopBit, const int c_iOddEvent);

	int SerialRecv(int serial_index, unsigned char* outBuf, int needlen, int timeout);//串口数据读取
	int SerialSend(int serial_index, unsigned char* inBuf, int bufLen);//串口数据写入

	bool ADGet(int AD_Index, float &outValue);//模拟量读取
    void DOAISet( int DO_Index, int status);  //模拟量口灯控制
	bool DIGet(int DI_Index, unsigned char &outValue);//开关量输入读取
    void DODISet( int DO_Index, int status);//开关量等状态控制

	void DOSet(int DO_Index, int outValue);//开关量输出
	void DOSetE(int DO_Index, int outValue);//继电器电源输出，由CADCBase的mode决定，如mode为1，则该函数有效
	static int f_getBaudRateEnum(unsigned int baudrate);
private:
	int m_iMode;
	int m_hDevHandle;
	float m_ADPara[4];
};
#endif

