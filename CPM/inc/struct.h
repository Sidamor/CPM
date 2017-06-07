#ifndef __STRUCT_SPI_API_H
#define __STRUCT_SPI_API_H

#ifndef succ
#define succ 0
#endif

#ifndef fail
#define fail -1
#endif


#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif


#define	BIT_6_IS_READ	(1<<6)
#define	BIT_6_IS_WRITE	(0<<6)

#define BIT_0	(0x01)
#define BIT_1	(0x01<<1)
#define BIT_2	(0x01<<2)
#define BIT_3	(0x01<<3)
#define BIT_4	(0x01<<4)
#define BIT_5	(0x01<<5)
#define BIT_6	(0x01<<6)
#define BIT_7	(0x01<<7)
 
//通信协议从1开始，保证字节不为0x00
typedef enum _MICROSYS_SPI_PROTOCAL_E
{
    E_SPI_ADC_0 = 1,//ADC设备有4个
    E_SPI_ADC_1,
    E_SPI_ADC_2,
    E_SPI_ADC_3,
    
    E_SPI_SERIAL_0,//串口设备有3个
    E_SPI_SERIAL_1,
    E_SPI_SERIAL_2,

    E_SPI_GPIO_IN_0,//GPIO 输入有4个     //PB6  G-DI1
    E_SPI_GPIO_IN_1,                     //PB7  G-DI2
    E_SPI_GPIO_IN_2,                     //PB8  G-DI3
    E_SPI_GPIO_IN_3,                     //PB9  G-DI4

    E_SPI_GPIO_AI_SET, //PC4    AI1-Light  //PC5    AI2-Light
                       //PB0    AI3-Light  //PB1    AI4-Light
                       //低位表示输出值BIT0~BIT3.对应位为1则置1. 0无效

    E_SPI_GPIO_AI_CLR, //PC4    AI1-Light  //PC5    AI2-Light
                   //PB0    AI3-Light  //PB1    AI4-Light
                       //低位表示输出值BIT0~BIT3.对应位为1则清0. 0无效
                
    E_SPI_GPIO_GDO_SET,     //PA8  G-DO1  //PC9  G-DO2  //PC8 G-DO3     
                            //PC7  G-DO4  //PB12    G-12VCC1 
                            //低位表示输出值BIT0~BIT4 对应位为1则置1. 0无效
                            
    E_SPI_GPIO_GDO_CLR,     //PA8  G-DO1  //PC9  G-DO2  //PC8 G-DO3     
                            //PC7  G-DO4  //PB12    G-12VCC1 
                            //低位表示输出值BIT0~BIT4对应位为1则清0. 0无效


    E_SPI_GPIO_DI_SET,      //PC10  DI4-Light  //PC11   DI3-Light
                            //PC12  DI2-Light  //PD2    DI1-Light
                            //低位表示输出值BIT0~BIT3对应位为1则置1. 0无效

    E_SPI_GPIO_DI_CLR,  //PC10  DI4-Light  //PC11   DI3-Light
                            //PC12  DI2-Light  //PD2    DI1-Light
                            //低位表示输出值BIT0~BIT3对应位为1则清0. 0无效
                            
    E_SPI_UART_BaudRate, //18
    E_SPI_UART_OTHERS,

    E_SPI_REG_ERROR,
    E_SPI_RTS_STATE,                //21
    E_SPI_MCU_VERSION,   //22
    
    E_SPI_FIFO_CLEAR,    //23

    E_SPI_RTC_Second, //24
    E_SPI_RTC_Minute,
    E_SPI_RTC_Hour,
    E_SPI_RTC_Date,
    E_SPI_RTC_Month,
    E_SPI_RTC_Day,   //29
    E_SPI_RTC_Year,  //30
}MICROSYS_SPI_PROTOCAL_E;

#endif



