#ifndef _ADC_SPI_API_
#define _ADC_SPI_API_ 
 
/**********************************************************************************
【名称】： adc_spi.c
【功能】： 供上层调用的Api
【修改】： 1.2011-05-18 1.0.0 Created By Microsys Access Gateway 
【相关】： 
**********************************************************************************/

#define DEBUG_SPI_API   0x1 //调试等级
 
//初始化
	int ApiSpiOpen(const char *c_pchPathName, int iFlags);

	int ApiSpiClose(const int c_iHandle);


	//读ADC 
	int ApiReadADC(const int c_iHandle, float *pfRecvBuf, const int c_iBufLen, const int c_iADCIndex);


	//读串口
	int ApiSerialRead(const int c_iHandle, unsigned char *pchRecvBuf, const int c_iBufLen, const int c_iSerialNum);

	//写串口
	int ApiSerialWrite(const int c_iHandle, unsigned char *pchSendBuf, const int c_iBufLen, const int c_iSerialNum);

	//读Gpio
	int ApiReadGpio(const int c_iHandle, unsigned char *pucRecvBuf, const int iBufLen, const int c_iGpioIndex);


	/**********************************************************************************
	【函数】： ApiWriteGpioAI
	【功能】： 设置GPIO  AI
	【参数】： 1.[in] const int c_iHandle,      
				2.[in] const int c_iFlag,       [0,1]      1-亮       0-灭
				3.[in] const int c_iAIIndex     [1,4]    
				//PC4  	AI1-Light  //PC5  	AI2-Light  //PB0  	AI3-Light  //PB1  	AI4-Light
				//低位表示输出值BIT0~BIT3.对应位为1则置1. 0无效
	【返回】： int , 实际写的字节数
	**********************************************************************************/
	int ApiWriteGpioAI(const int c_iHandle, const int c_iFlag, const int c_iAIIndex);



	/**********************************************************************************
	【函数】： ApiWriteGpioGdo
	【功能】： 设置GPIO  GDO
	【参数】： 1.[in] const int c_iHandle,      
								   2.[in] const int c_iData,            [0,1]      1-亮       0-灭
								   3.[in] const int c_iGdoIndex     [1,8]    
									//PA8  G-DO1  //PC9  G-DO2 //PC8 G-DO3	 //PC7  G-DO4 //PB12 	G-12VCC1 
									//RS485L-1       //RS485L-2      //RS485L-3
					//低位表示输出值BIT0~BIT4 对应位为1则置1. 0无效
	                               
	【返回】： int , 实际写的字节数
	**********************************************************************************/
	int ApiWriteGpioGdo(const int c_iHandle, const int c_iFlag, const int c_iGdoIndex);



	/**********************************************************************************
	【函数】： ApiWriteGpioDI
	【功能】： 设置GPIO  DI
	【参数】： 1.[in] const int c_iHandle,      
				2.[in] const int c_iData,       [0,1]      1-亮       0-灭
				3.[in] const int c_iAIIndex     [1,4]    
				//PC10  DI4-Light  //PC11  DI3-Light  //PC12  DI2-Light  //PD2  DI1-Light
				//低位表示输出值BIT0~BIT3对应位为1则置1. 0无效
	                               
	【返回】： int , 实际写的字节数
	**********************************************************************************/
	int ApiWriteGpioDI(const int c_iHandle, const int c_iFlag, const int c_iDIIndex);




	/**********************************************************************************
	【函数】： ApiWriteBaudRate
	【功能】： 设置波特率
	【参数】： 1.[in] const int c_iHandle,      
								   2.[in] const unsigned char c_ucIndex,         [1,8] 
								   低8为值表示要设置的BaudRate   不设置情况下默认9600
								   1  '1200
								   2  '2400
								   3  '4800
								   4  '9600
								   5  '19200
								   6  '38400
								   7  '57600
								   8  '115200
								   其它'9600
								   3.[in] const int c_iSerialIndex      {0,1,2} ------ {串口1，串口2，串口3}   
	【返回】： int , 实际写的字节数
	【修改】： 1.2011-05-19  1.0.0 Created By Microsys Access Gateway 
								   2.2011-05-19  1.0.0 Edit By ZhouX 
	【相关】： 
	**********************************************************************************/
	int ApiWriteBaudRate(const int c_iHandle, const int c_iBaudRateIndex, const int c_iSerialIndex);




	/**********************************************************************************
	【函数】： ApiWriteOthers
	【功能】： 设置停止位[1,2]
	【参数】： 1.[in] const int c_iHandle,      
								   2.[in] const int c_iStopBit,         [1,2] 
								   3.[in] const int c_iOddEvent     {0,1,2}----- {无，奇，偶}
								   低8为值表示要设置的参数   不设置情况下默认停止位1，无奇偶校验
								   停止位   Bit1  Bit0
									 1       0    0
									 2       0    1
									 1       其它
	                               
								   奇偶位   Bit3  Bit2
									 无      0    0
									 奇      0    1
									 偶      1    0
									 无       其它
								   4.[in] const int c_iSerialIndex      {0,1,2} ------ {串口1，串口2，串口3}
	【返回】： int , 实际写的字节数
	**********************************************************************************/
	int ApiWriteOthers(const int c_iHandle, const int c_iStopBit, const int c_iOddEvent, const int c_iSerialIndex);




	/**********************************************************************************
	【函数】： ApiReadRts
	【功能】： 读RTS
					其中RTS状态有三个，分别代表三个串口，用低8位来表示。
					BIT0, BIT1, BIT2分别代表三个状态位。1为高，0为低
	【参数】： 1.[in] const int c_iHandle,      
								   2.[in] unsigned char *pucRecvBuf,     读取的数据存放的地址   
								   3.[in] const int iBufLen    想要读取的字节数【1】
	【返回】： int , 实际读取到的字节数                               
	**********************************************************************************/
	int ApiReadRts(const int c_iHandle, unsigned char *pucRecvBuf, const int iBufLen);




	/**********************************************************************************
	【函数】： ApiReadMcuVersion
	【功能】： 读Mcu版本
	【参数】： 1.[in] const int c_iHandle,      
								   2.[in] unsigned char *pucRecvBuf,     读取的数据存放的地址   
								   3.[in] const int iBufLen    想要读取的字节数【1】
	【返回】： int , 实际读取到的字节数                               
	**********************************************************************************/
	int ApiReadMcuVersion(const int c_iHandle, unsigned char *pucRecvBuf, const int iBufLen);







	/**********************************************************************************
	【函数】： ApiClearFIFO
	【功能】： 清空arm7缓存
	【参数】： 1.[in] const int c_iHandle,      
	【返回】： int , 实际写的字节数
	【修改】： 1.2011-05-31  1.0.0 Created By ZhouX
	【相关】： 
	**********************************************************************************/
	int ApiClearFIFO(const int c_iHandle);


	/**********************************************************************************
	【函数】： ApiReadRTC
	【功能】： 
	【参数】：  1.[in]      const int c_iHandle,      
									2.[out]   unsigned char *ucRecvBuf,  
									3.[in]      const int c_iRtcIndex,  
	【返回】： int , 实际读到的字节数
	【修改】： 1.2011-06-14  1.0.0 Created By ZhouX
	【相关】： c_iRtcIndex [0, 6]   分别代表
	enum{
		E_Second = 0, 
		E_Minute,
		E_Hour,
		E_Date,
		E_Month,
		E_Day,   
		E_Year,  
	}
	**********************************************************************************/
	int ApiReadRTC(const int c_iHandle, const int c_iRtcIndex, unsigned char *ucRecvBuf);



	/**********************************************************************************
	【函数】： ApiWriteRTC
	【功能】： 
	【参数】： 1.[in] const int c_iHandle,      
	【返回】： int , 实际写的字节数
	【修改】： 1.2011-06-14  1.0.0 Created By ZhouX
	【相关】： c_iRtcIndex [0, 6]   分别代表
	enum{
		E_Second = 0, 
		E_Minute,
		E_Hour,
		E_Date,
		E_Month,
		E_Day,   
		E_Year,  
	}
	**********************************************************************************/
	int ApiWriteRTC(const int c_iHandle, const int c_iRtcIndex, const unsigned char c_ucValue);


	int ApiSpiUpdateRead(const int c_iHandle, unsigned char *pchRecvBuf, const int c_iBufLen, const int c_iSerialNum);
	int ApiSpiUpdateWrite(const int c_iHandle, unsigned char *pchSendBuf, const int c_iBufLen, const int c_iSerialNum);

	int ApiClearFIFONew(const int c_iHandle, const int c_index);
#endif

