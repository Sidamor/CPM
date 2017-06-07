#include "Define.h"
#include "EmApi.h"
#include "LightCtrl.h"


//局部宏定义
int	m_hdlGPIO;	/*GPIO接口句柄*/
CADCBase* m_pCADCBaseA;
CADCBase* m_pCADCBaseB;


CLightCtrl::CLightCtrl()
{
	m_hdlGPIO = -1;
	m_pCADCBaseA = NULL;
	m_pCADCBaseB = NULL;
}

CLightCtrl::~CLightCtrl(void)
{			

}

bool CLightCtrl::Initialize(int hGPIO, CADCBase* pA, CADCBase* pB)
{
	m_hdlGPIO = hGPIO;
	m_pCADCBaseA = pA;
	m_pCADCBaseB = pB;
	return true;
}

void CLightCtrl::BackLightSet( int portNum, int status )
{
    switch( portNum )
    {
    //----------------------485区域-------------------------------
    case INTERFACE_RS485_1:
        EMApiSetCts2(m_hdlGPIO, !status);
        break;
    case INTERFACE_RS485_2:
        EMApiSetCts1(m_hdlGPIO, !status);
        break;
    case INTERFACE_RS485_4:
        m_pCADCBaseA->DOSet(7, status);
        break;
    case INTERFACE_RS485_6:
        m_pCADCBaseA->DOSet(8, status);
        break;
    case INTERFACE_RS485_8:
        m_pCADCBaseA->DOSet(6, status);
        break;
    case INTERFACE_RS485_3:
        m_pCADCBaseB->DOSet(7, status);
        break;
    case INTERFACE_RS485_5:
        m_pCADCBaseB->DOSet(8, status);
        break;
    case INTERFACE_RS485_7:
        m_pCADCBaseB->DOSet(6, status);
        break;
    //------------------模拟量区域-------------------
    case INTERFACE_AD_A0:
        m_pCADCBaseA->DOAISet(1, status);
        break;
    case INTERFACE_AD_A1:
        m_pCADCBaseA->DOAISet(2, status);
        break;
    case INTERFACE_AD_A2:
        m_pCADCBaseA->DOAISet(3, status);
        break;
    case INTERFACE_AD_A3:
        m_pCADCBaseA->DOAISet(4, status);
        break;
    case INTERFACE_AD_B0:
        m_pCADCBaseB->DOAISet(1, status);
        break;
    case INTERFACE_AD_B1:
        m_pCADCBaseB->DOAISet(2, status);
        break;
    case INTERFACE_AD_B2:
        m_pCADCBaseB->DOAISet(3, status);
        break;
    case INTERFACE_AD_B3:
        m_pCADCBaseB->DOAISet(4, status);
        break;
    //-------------------开关量区域----------------------
    case INTERFACE_DIN_A0:
        m_pCADCBaseA->DODISet(1, status);
        break;
    case INTERFACE_DIN_A1:
        m_pCADCBaseA->DODISet(2, status);
        break;
    case INTERFACE_DIN_A2:
        m_pCADCBaseA->DODISet(3, status);
        break;
    case INTERFACE_DIN_A3:
        m_pCADCBaseA->DODISet(4, status);
        break;
    case INTERFACE_DIN_B0:
        m_pCADCBaseB->DODISet(1, status);
        break;
    case INTERFACE_DIN_B1:
        m_pCADCBaseB->DODISet(2, status);
        break;
    case INTERFACE_DIN_B2:
        m_pCADCBaseB->DODISet(3, status);
        break;
    case INTERFACE_DIN_B3:
        m_pCADCBaseB->DODISet(4, status);
        break;
    default:
        break;
    }
}
void CLightCtrl::AllLightOFF()
{
	//----------------485区灯状态---------------------
	BackLightSet(INTERFACE_RS485_1,  LIGHT_OFF);            //系统串口灯状态反置
	BackLightSet(INTERFACE_RS485_2,  LIGHT_OFF);
	BackLightSet(INTERFACE_RS485_3, LIGHT_OFF);
	BackLightSet(INTERFACE_RS485_4, LIGHT_OFF);
	BackLightSet(INTERFACE_RS485_5, LIGHT_OFF);
	BackLightSet(INTERFACE_RS485_6, LIGHT_OFF);
	BackLightSet(INTERFACE_RS485_7, LIGHT_OFF);
	BackLightSet(INTERFACE_RS485_8, LIGHT_OFF);
	//-----------------模拟量区灯状态------------------
	BackLightSet(INTERFACE_AD_A0, LIGHT_OFF);
	BackLightSet(INTERFACE_AD_A1, LIGHT_OFF);
	BackLightSet(INTERFACE_AD_A2, LIGHT_OFF);
	BackLightSet(INTERFACE_AD_A3, LIGHT_OFF);
	BackLightSet(INTERFACE_AD_B0, LIGHT_OFF);
	BackLightSet(INTERFACE_AD_B1, LIGHT_OFF);
	BackLightSet(INTERFACE_AD_B2, LIGHT_OFF);
	BackLightSet(INTERFACE_AD_B3, LIGHT_OFF);
	//------------------开关量区灯状态-----------------
	BackLightSet(INTERFACE_DIN_A0, LIGHT_OFF);
	BackLightSet(INTERFACE_DIN_A1, LIGHT_OFF);
	BackLightSet(INTERFACE_DIN_A2, LIGHT_OFF);
	BackLightSet(INTERFACE_DIN_A3, LIGHT_OFF);
	BackLightSet(INTERFACE_DIN_B0, LIGHT_OFF);
	BackLightSet(INTERFACE_DIN_B1, LIGHT_OFF);
	BackLightSet(INTERFACE_DIN_B2, LIGHT_OFF);
	BackLightSet(INTERFACE_DIN_B3, LIGHT_OFF);
	//-------------------前面板灯状态------------------
	EMApiSetDebug(m_hdlGPIO, LIGHT_OFF);      //系统灯
	//EMApiGsmPowerOff(m_hdlGPIO);      //GSM灯
	EMApiSetRF(m_hdlGPIO, LIGHT_OFF);    //433
	EMApiSetPLA(m_hdlGPIO, LIGHT_OFF);   //ALM
	EMApiSetALM(m_hdlGPIO, LIGHT_OFF);   //PLA
}


void CLightCtrl::AllLightON()
{
	//----------------485区灯状态---------------------
	BackLightSet(INTERFACE_RS485_1,  LIGHT_ON);            //系统串口灯状态反置
	BackLightSet(INTERFACE_RS485_2,  LIGHT_ON);
	BackLightSet(INTERFACE_RS485_3, LIGHT_ON);
	BackLightSet(INTERFACE_RS485_4, LIGHT_ON);
	BackLightSet(INTERFACE_RS485_5, LIGHT_ON);
	BackLightSet(INTERFACE_RS485_6, LIGHT_ON);
	BackLightSet(INTERFACE_RS485_7, LIGHT_ON);
	BackLightSet(INTERFACE_RS485_8, LIGHT_ON);
	//-----------------模拟量区灯状态------------------
	BackLightSet(INTERFACE_AD_A0, LIGHT_ON);
	BackLightSet(INTERFACE_AD_A1, LIGHT_ON);
	BackLightSet(INTERFACE_AD_A2, LIGHT_ON);
	BackLightSet(INTERFACE_AD_A3, LIGHT_ON);
	BackLightSet(INTERFACE_AD_B0, LIGHT_ON);
	BackLightSet(INTERFACE_AD_B1, LIGHT_ON);
	BackLightSet(INTERFACE_AD_B2, LIGHT_ON);
	BackLightSet(INTERFACE_AD_B3, LIGHT_ON);
	//------------------开关量区灯状态-----------------
	BackLightSet(INTERFACE_DIN_A0, LIGHT_ON);
	BackLightSet(INTERFACE_DIN_A1, LIGHT_ON);
	BackLightSet(INTERFACE_DIN_A2, LIGHT_ON);
	BackLightSet(INTERFACE_DIN_A3, LIGHT_ON);
	BackLightSet(INTERFACE_DIN_B0, LIGHT_ON);
	BackLightSet(INTERFACE_DIN_B1, LIGHT_ON);
	BackLightSet(INTERFACE_DIN_B2, LIGHT_ON);
	BackLightSet(INTERFACE_DIN_B3, LIGHT_ON);
	//-------------------前面板灯状态------------------
	EMApiSetDebug(m_hdlGPIO, LIGHT_ON);      //系统灯
	//EMApiGsmPowerOff(m_hdlGPIO);      //GSM灯
	EMApiSetRF(m_hdlGPIO, LIGHT_ON);    //433
	EMApiSetPLA(m_hdlGPIO, LIGHT_ON);   //ALM
	EMApiSetALM(m_hdlGPIO, LIGHT_ON);   //PLA
}
//GSM通断电
void CLightCtrl::GSMLightOFF()
{
	EMApiGsmPowerOff(m_hdlGPIO);
	//printf("GSMLightOFF Ret[%d]\n", ret);
}
void CLightCtrl::GSMLightON()
{
	EMApiGsmPowerOn(m_hdlGPIO);
	//printf("GSMLightON Ret[%d]\n", ret);
}
//系统灯
void CLightCtrl::DBGLightOFF()
{
	EMApiSetDebug(m_hdlGPIO, LIGHT_OFF);
}
void CLightCtrl::DBGLightON()
{
	EMApiSetDebug(m_hdlGPIO, LIGHT_ON);
}
//平台灯
void CLightCtrl::PLALightOFF()
{
	EMApiSetPLA(m_hdlGPIO, LIGHT_OFF);
}
void CLightCtrl::PLALightON()
{
	EMApiSetPLA(m_hdlGPIO, LIGHT_ON);
}
//其他函数
int CLightCtrl::GetGD0()
{
	return EMApiGetGDO0(m_hdlGPIO);
}
